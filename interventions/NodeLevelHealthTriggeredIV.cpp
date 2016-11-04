/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "NodeLevelHealthTriggeredIV.h"

#include "InterventionFactory.h"
#include "Log.h"
#include "Debug.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "EventTrigger.h"
#include "SimulationConfig.h"  // for verifying string triggers are 'valid'
#include "IIndividualHuman.h"

static const char * _module = "NodeLevelHealthTriggeredIV";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(NodeLevelHealthTriggeredIV)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IIndividualEventObserver)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(NodeLevelHealthTriggeredIV)

    IMPLEMENT_FACTORY_REGISTERED(NodeLevelHealthTriggeredIV)

    NodeLevelHealthTriggeredIV::NodeLevelHealthTriggeredIV()
    : parent(nullptr)
    , m_trigger_conditions()
    , max_duration(0)
    , duration(0)
    //, demographic_restrictions(true,TargetDemographicType::ExplicitAgeRanges)
    , demographic_restrictions()
    , m_disqualified_by_coverage_only(false)
    , blackout_period(0.0)
    , blackout_time_remaining(0.0)
    , blackout_event_trigger("UNINITIALIZED STRING")
    , notification_occured(false)
    , event_occured_map()
    , event_occurred_while_resident_away()
    , actual_individual_intervention_config()
    , actual_node_intervention_config()
    , _di(nullptr)
    , _ndi(nullptr)
    , using_individual_config(false)
    {
    }

    NodeLevelHealthTriggeredIV::~NodeLevelHealthTriggeredIV() { }
    int NodeLevelHealthTriggeredIV::AddRef()
    {
        return BaseNodeIntervention::AddRef();
    }
    int NodeLevelHealthTriggeredIV::Release()
    {
        return BaseNodeIntervention::Release();
    }

    bool
    NodeLevelHealthTriggeredIV::ConfigureTriggers(
        const Configuration* inputJson
    )
    {
        // --------------------------------------------------------------------------------------------------------------------
        // --- Phase 0 - Pre V2.0 - Trigger_Condition was an enum and users had to have all indivual events in the enum list.
        // --- Phase 1 -     V2.0 - Trigger_Condition was an enum but TriggerString and TriggerList were added to allow the 
        // ---                      user to use new User Defined events (i.e. strings).
        // --- Phase 2 - 11/12/15 - Trigger_Condition is now a string and so users don't need to use Trigger_Condition_String
        // ---                      in order to use the User Defined events.  However, we are leaving support here for 
        // ---                      Trigger_Condition_String for backward compatibility (and I'm lazy).
        // --- Phase 3 - TBD      - In the future, we want to consolidate so that the user only defines Trigger_Condition_List
        // --------------------------------------------------------------------------------------------------------------------
        JsonConfigurable::_useDefaults = InterventionFactory::useDefaults;

        std::string trigger_string_enum_str = IndividualEventTriggerType::pairs::lookup_key( IndividualEventTriggerType::TriggerString );
        std::string trigger_list_enum_str   = IndividualEventTriggerType::pairs::lookup_key( IndividualEventTriggerType::TriggerList );

        jsonConfigurable::tDynamicStringSet tmp_listed_events;
        if( !JsonConfigurable::_dryrun )
        {
            // ------------------------------------------------------------------
            // --- Can't do this during "dryrun" because SimulationConfig is null
            // ------------------------------------------------------------------
            tmp_listed_events = GET_CONFIGURABLE(SimulationConfig)->listed_events;
            tmp_listed_events.insert( trigger_string_enum_str );
            tmp_listed_events.insert( trigger_list_enum_str );
        }

        jsonConfigurable::ConstrainedString trigger_condition = NO_TRIGGER_STR;
        trigger_condition.constraints = "<configuration>:Listed_Events.*";
        trigger_condition.constraint_param = &tmp_listed_events;

        EventTrigger trigger_condition_string = NO_TRIGGER_STR;

        // TODO need to add conditionality but not supported in all datatypes yet
        initConfigTypeMap( "Trigger_Condition", &trigger_condition, HTI_Trigger_Condition_DESC_TEXT  );

        bool retValue = JsonConfigurable::Configure( inputJson );

        if( retValue && ( (trigger_condition == trigger_list_enum_str  ) ||
                          (trigger_condition == trigger_string_enum_str) ||
                          JsonConfigurable::_dryrun ) )
        {
            if( trigger_condition == trigger_list_enum_str || JsonConfigurable::_dryrun )
            {
                // TODO need to add conditionality but not supported in all datatypes yet
                initConfigTypeMap( "Trigger_Condition_List", &m_trigger_conditions, NLHTI_Trigger_Condition_List_DESC_TEXT );
            }
            // would have else but schema needs to be able to enter both blocks. 
            if( trigger_condition == trigger_string_enum_str || JsonConfigurable::_dryrun )
            {
                // TODO need to add conditionality but not supported in all datatypes yet
                initConfigTypeMap( "Trigger_Condition_String", &trigger_condition_string, NLHTI_Trigger_Condition_String_DESC_TEXT  );
            }
            // ------------------------------------------------------------------------------------------------------------
            // --- We have to call JsonConfigurable::Configure() twice in order to get the value of Trigger_Condition and
            // --- then read in Trigger_Condition_List or Trigger_Condition_String based on the value of Trigger_Condition.
            // ------------------------------------------------------------------------------------------------------------
            retValue = JsonConfigurable::Configure( inputJson );

            if( retValue )
            {
                if( trigger_condition == trigger_string_enum_str )
                {
                    m_trigger_conditions.push_back( trigger_condition_string );
                    LOG_INFO_F( "This NLHTI is listening to %s events.\n", trigger_condition_string.c_str() );
                }
                else if( trigger_condition == trigger_list_enum_str )
                {
                    // ------------------------------------------------------------------------------
                    // --- Use a constrained string to verify that the strings in the list are valid.
                    // --- Assignign the trigger to the constrainged string will do the validation
                    // ------------------------------------------------------------------------------
                    EventTrigger validating_trigger = NO_TRIGGER_STR;
                    for( auto trigger : m_trigger_conditions )
                    {
                        validating_trigger = trigger; // validate
                    }
                }
            }
        }
        else if( retValue && (trigger_condition != trigger_string_enum_str) &&
                             (trigger_condition != NO_TRIGGER_STR         ) )
        {
            m_trigger_conditions.push_back( trigger_condition );
            LOG_INFO_F( "This NLHTI is listening to %s events.\n", m_trigger_conditions[0].c_str() );
        }
        return retValue;
    }

    bool
    NodeLevelHealthTriggeredIV::Configure(
        const Configuration * inputJson
    )
    {
        JsonConfigurable::_useDefaults = InterventionFactory::useDefaults;

        if( JsonConfigurable::_dryrun || inputJson->Exist( "Actual_NodeIntervention_Config" ) )
        {
            initConfigComplexType( "Actual_NodeIntervention_Config", &actual_node_intervention_config, BT_Actual_NodeIntervention_Config_DESC_TEXT );
        }
        if( JsonConfigurable::_dryrun || inputJson->Exist( "Actual_IndividualIntervention_Config" ) )
        {
            initConfigComplexType( "Actual_IndividualIntervention_Config", &actual_individual_intervention_config, BT_Actual_IndividualIntervention_Config_DESC_TEXT );
        }
        if( !JsonConfigurable::_dryrun && 
            ( ( inputJson->Exist( "Actual_IndividualIntervention_Config" ) &&  inputJson->Exist( "Actual_NodeIntervention_Config" )) || 
              (!inputJson->Exist( "Actual_IndividualIntervention_Config" ) && !inputJson->Exist( "Actual_NodeIntervention_Config" )) ) )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "You must define either 'Actual_IndividualIntervention_Config' or 'Actual_NodeIntervention_Config' but not both." );
        }

        initConfigTypeMap("Duration", &max_duration, BT_Duration_DESC_TEXT, -1.0f, FLT_MAX, -1.0f ); // -1 is a convention for indefinite duration

        initConfigTypeMap( "Blackout_Period", &blackout_period, Blackout_Period_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "Blackout_Event_Trigger", &blackout_event_trigger, Blackout_Event_Trigger_DESC_TEXT );

        demographic_restrictions.ConfigureRestrictions( this, inputJson );

        bool retValue = ConfigureTriggers( inputJson );

        //this section copied from standardevent coordinator
        if( retValue && !JsonConfigurable::_dryrun )
        {
            demographic_restrictions.CheckConfiguration();
            if( inputJson->Exist( "Actual_IndividualIntervention_Config" ) )
            {
                InterventionValidator::ValidateIntervention( actual_individual_intervention_config._json );
                using_individual_config = true;
            }
            else if( inputJson->Exist( "Actual_NodeIntervention_Config" ) )
            {
                InterventionValidator::ValidateIntervention( actual_node_intervention_config._json );
                using_individual_config = false;
            }
        }
        JsonConfigurable::_useDefaults = false;
        return retValue;    
    }

    bool
    NodeLevelHealthTriggeredIV::Distribute(
        INodeEventContext *pNodeEventContext,
        IEventCoordinator2 *pEC
    )
    {
        LOG_DEBUG_F("Distributed Nodelevel health-triggered intervention to NODE: %d\n", pNodeEventContext->GetId().data);

        // QI to register ourself as a NodeLevelHealthTriggeredIV observer
        INodeTriggeredInterventionConsumer * pNTIC = nullptr;
        if (s_OK != pNodeEventContext->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNodeEventContext", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
        }
        release_assert( pNTIC );
        for( auto &trigger : m_trigger_conditions )
        {
            pNTIC->RegisterNodeEventObserverByString( (IIndividualEventObserver*)this, trigger );
        }

        // We can QI NEC to get the campaign cost observer (it's just the node!)
        ICampaignCostObserver *iCCO;
        if (parent && s_OK != parent->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&iCCO))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "ICampaignCostObserver", "INodeEventContext" );
        }
        assert( iCCO );
        return true;
    }

    //returns false if didn't get the intervention
    bool NodeLevelHealthTriggeredIV::notifyOnEvent(
        IIndividualHumanEventContext *pIndiv,
        const std::string& StateChange
    )
    {
        IIndividualHuman *p_human = nullptr;
        if (s_OK != pIndiv->QueryInterface(GET_IID(IIndividualHuman), (void**)&p_human))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIndiv", "IIndividualHuman", "IIndividualHumanEventContext" );
        }

        bool missed_intervention = false ;
        if( StateChange == IndividualEventTriggerType::pairs::lookup_key( IndividualEventTriggerType::Emigrating ) )
        {
            if( p_human->AtHome() )
            {
                // ------------------------------------------------------------------------------------------------
                // --- If the individual is leaving his node of residence, then we want to keep track that he left
                // --- so that when he returns we can give him the interventions that he missed.
                // ------------------------------------------------------------------------------------------------
                release_assert( event_occurred_while_resident_away.count( pIndiv->GetSuid() ) == 0 );
                event_occurred_while_resident_away.insert( make_pair( pIndiv->GetSuid(), false ) );
            }
            return false ;
        }
        else if( StateChange == IndividualEventTriggerType::pairs::lookup_key( IndividualEventTriggerType::Immigrating ) )
        {
            if( p_human->AtHome() )
            {
                // ------------------------------------------------------------------------------
                // --- If the individual has returned home and they missed the intervention, then
                // --- we want them to get it, assuming they qualify.
                // ------------------------------------------------------------------------------
                release_assert( event_occurred_while_resident_away.count( pIndiv->GetSuid() ) > 0 );
                missed_intervention = event_occurred_while_resident_away[ pIndiv->GetSuid() ] ;
                event_occurred_while_resident_away.erase( pIndiv->GetSuid() );
                if( missed_intervention )
                {
                    LOG_DEBUG_F( "Resident %d came home and intervention was distributed while away.\n", pIndiv->GetSuid().data );
                }
            }

            if( !missed_intervention )
            {
                return false ;
            }
        }
        else // the trigger event
        {
            // --------------------------------------------------------------------------------------------
            // --- If this is one of the non-migrating events that they intervention is listening for
            // --- and this is not a blackout period, then record that the residents that are away missed
            // --- the intervention.
            // --------------------------------------------------------------------------------------------
            if( blackout_time_remaining <= 0.0f )
            {
                for( auto& rEntry : event_occurred_while_resident_away )
                {
                    rEntry.second = true ;
                }
            }
        }

        if( !blackout_event_trigger.IsUninitialized() && (blackout_event_trigger != NO_TRIGGER_STR ) && (blackout_period > 0.0) )
        {
            if( (event_occured_map[ StateChange ].count( pIndiv->GetSuid().data ) > 0) || (!missed_intervention && (blackout_time_remaining > 0.0f)) )
            {
                INodeTriggeredInterventionConsumer * pNTIC = NULL;
                if (s_OK != parent->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC) )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
                }
                pNTIC->TriggerNodeEventObserversByString( pIndiv, blackout_event_trigger );
                return false;
            }
        }

        LOG_DEBUG_F("Individual %d experienced event %s, check to see if they pass the conditions before distributing actual_intervention \n",
                    pIndiv->GetInterventionsContext()->GetParent()->GetSuid().data,
                    StateChange.c_str()
                   );

        assert( parent );
        assert( parent->GetRng() );

        bool distributed = false;
        if( _di != nullptr )
        {
            //initialize this flag by individual (not by node)
            m_disqualified_by_coverage_only = false;

            if( qualifiesToGetIntervention( pIndiv ) == false )
            {
                LOG_DEBUG_F("Individual failed to qualify for intervention, m_disqualified_by_coverage_only is %d \n", m_disqualified_by_coverage_only);
                if (m_disqualified_by_coverage_only == true)
                {
                    onDisqualifiedByCoverage( pIndiv );
                }
                return false;
            }

            // Query for campaign cost observer interface from INodeEventContext *parent
            ICampaignCostObserver *iCCO;
            if (s_OK != parent->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&iCCO))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "ICampaignCostObserver", "INodeEventContext" );
            }

            // Huge performance win by cloning instead of configuring.
            IDistributableIntervention *di = _di->Clone();
            release_assert( di );
            di->AddRef();

            distributed = di->Distribute( pIndiv->GetInterventionsContext(), iCCO );
            if( distributed )
            {
                std::string classname = GetInterventionClassName();
                LOG_DEBUG_F("A Node level health-triggered intervention (%s) was successfully distributed to individual %d\n",
                            classname.c_str(),
                            pIndiv->GetInterventionsContext()->GetParent()->GetSuid().data
                           );
            }
            else
            {
                LOG_DEBUG_F( "Intervention not distributed?\n" );
            }
            di->Release();
        }
        else
        {
            release_assert( _ndi );

            // Huge performance win by cloning instead of configuring.
            INodeDistributableIntervention *ndi = _ndi->Clone();
            release_assert( ndi );
            ndi->AddRef();

            distributed =  ndi->Distribute( parent, nullptr );

            if( distributed )
            {
                std::string classname = GetInterventionClassName();
                LOG_INFO_F("Distributed '%s' intervention to node %d\n", classname.c_str(), parent->GetExternalId() );
            }
            ndi->Release();
        }

        if( distributed )
        {
            notification_occured = true ;
            event_occured_map[ StateChange ].insert( pIndiv->GetSuid().data ); 
        }

        return distributed;
    }

    void NodeLevelHealthTriggeredIV::Update( float dt )
    {
        duration += dt;
        if( max_duration >= 0 && duration > max_duration )
        {
            // QI to register ourself as a node level health triggered observer
            LOG_DEBUG_F( "Node-Level HTI reached max_duration. Time to unregister...\n" );
            INodeTriggeredInterventionConsumer * pNTIC = nullptr;
            if (s_OK == parent->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC) )
            {
                release_assert( pNTIC );
                for( auto &trigger : m_trigger_conditions )
                {
                    pNTIC->UnregisterNodeEventObserverByString( this, trigger );
                }
                expired = true ;
            }
            else
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
        }
        event_occured_map.clear();
        blackout_time_remaining -= dt ;
        if( notification_occured )
        {
            notification_occured = false ;
            blackout_time_remaining = blackout_period ;
            LOG_DEBUG_F( "start blackout period - nodeid=%d\n",parent->GetExternalId() );
        }
    }

    void NodeLevelHealthTriggeredIV::SetContextTo(INodeEventContext *context)
    {
        release_assert( context );
        parent = context;

        // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
        //const IInterventionFactory* ifobj = dynamic_cast<NodeEventContextHost *>(parent)->GetInterventionFactoryObj();
        IGlobalContext *pGC = nullptr;
        const IInterventionFactory* ifobj = nullptr;
        if (s_OK == parent->QueryInterface(GET_IID(IGlobalContext), (void**)&pGC))
        {
            ifobj = pGC->GetInterventionFactory();
        }
        if (!ifobj)
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The pointer to IInterventionFactory object is not valid (could be DLL specific)" );
        }
        if( (_di == nullptr) && (_ndi == nullptr) )
        {
            Configuration* config = nullptr;
            if( using_individual_config )
            {
                config = Configuration::CopyFromElement( (actual_individual_intervention_config._json) );
            }
            else
            {
                config = Configuration::CopyFromElement( (actual_node_intervention_config._json) );
            }

            _di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention( config );

            if( _di == nullptr )
            {
                _ndi = const_cast<IInterventionFactory*>(ifobj)->CreateNDIIntervention( config );
            }
            release_assert( (_di !=nullptr) || (_ndi != nullptr) );

            delete config;
            config = nullptr;
        }
    }

    // private/protected
    bool
    NodeLevelHealthTriggeredIV::qualifiesToGetIntervention(
        const IIndividualHumanEventContext * const pIndividual
    )
    {
        bool retQualifies = demographic_restrictions.IsQualified( pIndividual );

        //OK they passed the property and age test, now check if they are part of the demographic coverage
        if (retQualifies)
        {
            LOG_DEBUG_F("demographic_coverage = %f\n", getDemographicCoverage());
            if( !SMART_DRAW( getDemographicCoverage() ) )
            {
                m_disqualified_by_coverage_only = true;
                LOG_DEBUG_F("Demographic coverage ruled this out, m_disqualified_by_coverage_only is %d \n", m_disqualified_by_coverage_only);
                retQualifies = false;
            }
        }

        LOG_DEBUG_F( "Returning %d from %s\n", retQualifies, __FUNCTION__ );
        return retQualifies;
    }

    float NodeLevelHealthTriggeredIV::getDemographicCoverage() const
    {
        return demographic_restrictions.GetDemographicCoverage();    
    }

    void NodeLevelHealthTriggeredIV::onDisqualifiedByCoverage( IIndividualHumanEventContext *pIndiv )
    {
        //do nothing, this is for the scale up switch
    }

    std::string NodeLevelHealthTriggeredIV::GetInterventionClassName() const
    {
        std::string class_name;
        if( using_individual_config )
        {
            class_name = std::string(json::QuickInterpreter( actual_individual_intervention_config._json )[ "class" ].As<json::String>());
        }
        else
        {
            class_name = std::string( json::QuickInterpreter( actual_node_intervention_config._json )[ "class" ].As<json::String>() );
        }
        return class_name;
    }
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, NodeLevelHealthTriggeredIV& iv, const unsigned int v)
    {
        ar & iv.actual_intervention_config;
        ar & iv.demographic_restrictions;
        ar & iv.efficacy;
        ar & iv.max_duration;
        ar & iv.m_trigger_conditions;
        ar & iv.blackout_period;
        ar & iv.blackout_time_remaining;
        ar & iv.blackout_event_trigger;
        ar & iv.notification_occured;
        ar & iv.event_occured_map;
        ar & iv.event_occurred_while_resident_away;
    }
}
#endif
