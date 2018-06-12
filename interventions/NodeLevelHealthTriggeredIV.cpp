/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
#include "IIndividualHuman.h"

SETUP_LOGGING( "NodeLevelHealthTriggeredIV" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(NodeLevelHealthTriggeredIV)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IIndividualEventObserver)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(NodeLevelHealthTriggeredIV)

    IMPLEMENT_FACTORY_REGISTERED(NodeLevelHealthTriggeredIV)

    NodeLevelHealthTriggeredIV::NodeLevelHealthTriggeredIV()
    : BaseNodeIntervention()
    , m_trigger_conditions()
    , max_duration(0)
    , duration(0)
    , node_property_restrictions()
    , demographic_restrictions()
    , m_disqualified_by_coverage_only(false)
    , blackout_period(0.0)
    , blackout_time_remaining(0.0)
    , blackout_event_trigger()
    , blackout_on_first_occurrence(false)
    , notification_occured(false)
    , distribute_on_return_home(false)
    , event_occured_list()
    , event_occurred_while_resident_away()
    , actual_individual_intervention_config()
    , actual_node_intervention_config()
    , _di(nullptr)
    , _ndi(nullptr)
    , using_individual_config(false)
    {
    }

    NodeLevelHealthTriggeredIV::~NodeLevelHealthTriggeredIV()
    { 
        delete _di;
        delete _ndi;
    }
    int NodeLevelHealthTriggeredIV::AddRef()
    {
        return BaseNodeIntervention::AddRef();
    }
    int NodeLevelHealthTriggeredIV::Release()
    {
        return BaseNodeIntervention::Release();
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
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "You must specify either 'Actual_IndividualIntervention_Config' or 'Actual_NodeIntervention_Config' (but do not specify both)." );
        }

        initConfigTypeMap( "Distribute_On_Return_Home", &distribute_on_return_home, Distribute_On_Return_Home_DESC_TEXT, false );

        initConfigTypeMap("Duration", &max_duration, BT_Duration_DESC_TEXT, -1.0f, FLT_MAX, -1.0f ); // -1 is a convention for indefinite duration

        initConfigTypeMap( "Blackout_Period", &blackout_period, Blackout_Period_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "Blackout_Event_Trigger", &blackout_event_trigger, Blackout_Event_Trigger_DESC_TEXT );
        initConfigTypeMap( "Blackout_On_First_Occurrence", &blackout_on_first_occurrence, Blackout_On_First_Occurrence_DESC_TEXT, false );

        initConfigComplexType( "Node_Property_Restrictions", &node_property_restrictions, NLHTIV_Node_Property_Restriction_DESC_TEXT );

        demographic_restrictions.ConfigureRestrictions( this, inputJson );

        // --------------------------------------------------------------------------------------------------------------------
        // --- Phase 0 - Pre V2.0 - Trigger_Condition was an enum and users had to have all indivual events in the enum list.
        // --- Phase 1 -     V2.0 - Trigger_Condition was an enum but TriggerString and TriggerList were added to allow the 
        // ---                      user to use new User Defined events (i.e. strings).
        // --- Phase 2 - 11/12/15 - Trigger_Condition is now a string and so users don't need to use Trigger_Condition_String
        // ---                      in order to use the User Defined events.  However, we are leaving support here for 
        // ---                      Trigger_Condition_String for backward compatibility (and I'm lazy).
        // --- Phase 3 - 11/9/16    Consolidate so that the user only defines Trigger_Condition_List
        // --------------------------------------------------------------------------------------------------------------------
        JsonConfigurable::_useDefaults = InterventionFactory::useDefaults; // Why???
        initConfigTypeMap( "Trigger_Condition_List", &m_trigger_conditions, NLHTI_Trigger_Condition_List_DESC_TEXT );

        bool retValue = BaseNodeIntervention::Configure( inputJson );

        //this section copied from standardevent coordinator
        if( retValue && !JsonConfigurable::_dryrun )
        {
            demographic_restrictions.CheckConfiguration();
            if( inputJson->Exist( "Actual_IndividualIntervention_Config" ) )
            {
                InterventionValidator::ValidateIntervention( GetTypeName(),
                                                             InterventionTypeValidation::INDIVIDUAL,
                                                             actual_individual_intervention_config._json,
                                                             inputJson->GetDataLocation() );
                using_individual_config = true;
            }
            else if( inputJson->Exist( "Actual_NodeIntervention_Config" ) )
            {
                InterventionValidator::ValidateIntervention( GetTypeName(),
                                                             InterventionTypeValidation::NODE,
                                                             actual_node_intervention_config._json, 
                                                             inputJson->GetDataLocation() );
                using_individual_config = false;
            }

            event_occured_list.resize( EventTriggerFactory::GetInstance()->GetNumEventTriggers() );

            bool blackout_configured = (inputJson->Exist("Blackout_Event_Trigger")) || (inputJson->Exist("Blackout_Period")) || (inputJson->Exist("Blackout_On_First_Occurrence"));
            bool blackout_all_configured = (inputJson->Exist("Blackout_Event_Trigger")) && (inputJson->Exist("Blackout_Period")) && (inputJson->Exist("Blackout_On_First_Occurrence"));
            if (blackout_configured && !blackout_all_configured)
            {
                std::vector<string> blackout_missing_str;
                if( !inputJson->Exist("Blackout_Event_Trigger") )       blackout_missing_str.push_back("Blackout_Event_Trigger");
                if( !inputJson->Exist("Blackout_Period") )              blackout_missing_str.push_back("Blackout_Period");
                if( !inputJson->Exist("Blackout_On_First_Occurrence") ) blackout_missing_str.push_back("Blackout_On_First_Occurrence");

                throw MissingParameterFromConfigurationException(__FILE__, __LINE__, __FUNCTION__, inputJson->GetDataLocation().c_str(), blackout_missing_str, " All three Blackout parameters must be configured.");
            }

            if( distribute_on_return_home )
            {
                if( (std::find( m_trigger_conditions.begin(), m_trigger_conditions.end(), EventTrigger::Emigrating ) != m_trigger_conditions.end()) ||
                    (std::find( m_trigger_conditions.begin(), m_trigger_conditions.end(), EventTrigger::Immigrating ) != m_trigger_conditions.end()) )
                {
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "When using Distribute_On_Return_Home, you cannot also use Emigrating or Immigrating." );
                }
                m_trigger_conditions.push_back( EventTrigger::Emigrating );
                m_trigger_conditions.push_back( EventTrigger::Immigrating );
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
        bool was_distributed = BaseNodeIntervention::Distribute(pNodeEventContext, pEC);
        if (was_distributed)
        {
            LOG_DEBUG_F("Distributed Nodelevel health-triggered intervention to NODE: %d\n", pNodeEventContext->GetId().data);

            // QI to register ourself as a NodeLevelHealthTriggeredIV observer
            INodeTriggeredInterventionConsumer * pNTIC = nullptr;
            if (s_OK != pNodeEventContext->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC))
            {
                throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNodeEventContext", "INodeTriggeredInterventionConsumer", "INodeEventContext");
            }
            release_assert(pNTIC);
            for (auto &trigger : m_trigger_conditions)
            {
                pNTIC->RegisterNodeEventObserver((IIndividualEventObserver*)this, trigger);
            }
        }
        return was_distributed;
    }

    //returns false if didn't get the intervention
    bool NodeLevelHealthTriggeredIV::notifyOnEvent(
        IIndividualHumanEventContext *pIndiv,
        const EventTrigger& trigger
    )
    {
        // ----------------------------------------------------------------------
        // --- Ignore events for nodes that don't qualify due to their properties
        // ----------------------------------------------------------------------
        if( !node_property_restrictions.Qualifies( parent->GetNodeContext()->GetNodeProperties() ) )
        {
            return false;
        }

        IIndividualHuman *p_human = nullptr;
        if (s_OK != pIndiv->QueryInterface(GET_IID(IIndividualHuman), (void**)&p_human))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIndiv", "IIndividualHuman", "IIndividualHumanEventContext" );
        }

        bool missed_intervention = false ;
        if( distribute_on_return_home && (trigger == EventTrigger::Emigrating) )
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
        else if( distribute_on_return_home && (trigger == EventTrigger::Immigrating) )
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

        if( !blackout_event_trigger.IsUninitialized() && (blackout_period > 0.0) )
        {
            if( (event_occured_list[ trigger.GetIndex() ].count( pIndiv->GetSuid().data ) > 0) || (!missed_intervention && (blackout_time_remaining > 0.0f)) )
            {
                INodeTriggeredInterventionConsumer * pNTIC = NULL;
                if (s_OK != parent->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC) )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
                }
                pNTIC->TriggerNodeEventObservers( pIndiv, blackout_event_trigger );
                return false;
            }
        }

        LOG_DEBUG_F("Individual %d experienced event %s, check to see if they pass the conditions before distributing actual_intervention \n",
                    pIndiv->GetInterventionsContext()->GetParent()->GetSuid().data,
                    trigger.c_str()
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
            if( blackout_on_first_occurrence )
            {
                blackout_time_remaining = blackout_period ;
            }
            else
            {
                notification_occured = true ;
            }
            event_occured_list[ trigger.GetIndex() ].insert( pIndiv->GetSuid().data ); 
        }

        return distributed;
    }


    void NodeLevelHealthTriggeredIV::Unregister()
    {
        // unregister ourself as a node level health triggered observer
        INodeTriggeredInterventionConsumer * pNTIC = nullptr;
        if (s_OK != parent->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC))
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "parent", "INodeTriggeredInterventionConsumer", "INodeEventContext");
        }
        release_assert(pNTIC);
        for (auto &trigger : m_trigger_conditions)
        {
            pNTIC->UnregisterNodeEventObserver( this, trigger );
        }
        SetExpired( true );
    }

    void NodeLevelHealthTriggeredIV::Update(float dt)
    {
        if (!BaseNodeIntervention::UpdateNodesInterventionStatus())
        {
            Unregister();
            return;
        }

        duration += dt;
        if( max_duration >= 0 && duration > max_duration )
        {
            LOG_DEBUG_F("Node-Level HTI reached max_duration. Time to unregister...\n");
            Unregister();
        }
        event_occured_list.clear();
        event_occured_list.resize( EventTriggerFactory::GetInstance()->GetNumEventTriggers() );

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
        BaseNodeIntervention::SetContextTo( context );

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
                config = Configuration::CopyFromElement( (actual_individual_intervention_config._json), "campaign" );
            }
            else
            {
                config = Configuration::CopyFromElement( (actual_node_intervention_config._json), "campaign" );
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
