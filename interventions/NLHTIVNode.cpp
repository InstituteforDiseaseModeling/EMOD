/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "NLHTIVNode.h"

#include "InterventionFactory.h"
#include "Log.h"
#include "Debug.h"
#include "ISimulationContext.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
//#include "IIndividualHuman.h"
#include "SimulationEventContext.h"

SETUP_LOGGING( "NLHTIVNode" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(NLHTIVNode)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(INodeEventObserver)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(NLHTIVNode)

    IMPLEMENT_FACTORY_REGISTERED(NLHTIVNode)

    NLHTIVNode::NLHTIVNode()
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
    , event_occured_list()
    , actual_node_intervention_config()
    , _ndi(nullptr)
    , using_individual_config(false)
    {
    }

    NLHTIVNode::~NLHTIVNode()
    { 
        delete _ndi;
    }
    int NLHTIVNode::AddRef()
    {
        return BaseNodeIntervention::AddRef();
    }
    int NLHTIVNode::Release()
    {
        return BaseNodeIntervention::Release();
    }

    bool
    NLHTIVNode::Configure(
        const Configuration * inputJson
    )
    {
        JsonConfigurable::_useDefaults = InterventionFactory::useDefaults;

        if( JsonConfigurable::_dryrun || inputJson->Exist( "Actual_NodeIntervention_Config" ) )
        {
            initConfigComplexType( "Actual_NodeIntervention_Config", &actual_node_intervention_config, BT_Actual_NodeIntervention_Config_DESC_TEXT );
        }
        if( !JsonConfigurable::_dryrun && 
            ( ( inputJson->Exist( "Actual_IndividualIntervention_Config" ) &&  inputJson->Exist( "Actual_NodeIntervention_Config" )) || 
              (!inputJson->Exist( "Actual_IndividualIntervention_Config" ) && !inputJson->Exist( "Actual_NodeIntervention_Config" )) ) )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "You must specify either 'Actual_IndividualIntervention_Config' or 'Actual_NodeIntervention_Config' (but do not specify both)." );
        }

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
            if( inputJson->Exist( "Actual_NodeIntervention_Config" ) )
            {
                InterventionValidator::ValidateIntervention( GetTypeName(),
                                                             InterventionTypeValidation::NODE,
                                                             actual_node_intervention_config._json, 
                                                             inputJson->GetDataLocation() );
                using_individual_config = false;
            }

            event_occured_list.resize( EventTriggerNodeFactory::GetInstance()->GetNumEventTriggers() );

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
        }
        JsonConfigurable::_useDefaults = false;
        return retValue;    
    }

    bool
    NLHTIVNode::Distribute(
        INodeEventContext *pNodeEventContext,
        IEventCoordinator2 *pEC
    )
    {
        bool was_distributed = BaseNodeIntervention::Distribute(pNodeEventContext, pEC);
        if (was_distributed)
        {
            LOG_DEBUG_F("Distributed Nodelevel health-triggered intervention to NODE: %d\n", pNodeEventContext->GetId().data);

            // QI to register ourself as a NLHTIVNode observer
            INodeEventBroadcaster * broadcaster = pNodeEventContext->GetNodeContext()->GetParent()->GetSimulationEventContext()->GetNodeEventBroadcaster();
            release_assert( broadcaster );
            for (auto &trigger : m_trigger_conditions)
            {
                LOG_DEBUG_F( "Registering as observer of event %s.\n", trigger.c_str() );
                broadcaster->RegisterObserver((INodeEventObserver*)this, trigger);
            }
        }
        else
        {
            LOG_WARN_F( "Failed to distribute intervention to node.\n" );
        }
        return was_distributed;
    }

    //returns false if didn't get the intervention
    bool NLHTIVNode::notifyOnEvent(
        INodeEventContext *pNode,
        const EventTriggerNode& trigger
    )
    {
        // ----------------------------------------------------------------------
        // --- Ignore events for nodes that don't qualify due to their properties
        // ----------------------------------------------------------------------
        if( !node_property_restrictions.Qualifies( parent->GetNodeContext()->GetNodeProperties() ) )
        {
            return false;
        }

        bool missed_intervention = false ;
        // the trigger event
        LOG_DEBUG_F("Node %d experienced event %s, check to see if it passes the conditions before distributing actual_intervention \n",
                    pNode->GetNodeContext()->GetSuid().data,
                    trigger.c_str()
                   );

        assert( parent );
        //assert( parent->GetRng() );

        release_assert( _ndi );

        // Huge performance win by cloning instead of configuring.
        INodeDistributableIntervention *ndi = _ndi->Clone();
        release_assert( ndi );
        ndi->AddRef();
        bool distributed =  ndi->Distribute( parent, nullptr );

        if( distributed )
        {
            std::string classname = GetInterventionClassName();
            LOG_INFO_F("Distributed '%s' intervention to node %d\n", classname.c_str(), parent->GetExternalId() );
        }
        ndi->Release();


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
            event_occured_list[ trigger.GetIndex() ].insert( pNode->GetNodeContext()->GetSuid().data ); 
        }

        return distributed;
    }


    void NLHTIVNode::Unregister()
    {
        // unregister ourself as a node level health triggered observer
        INodeEventBroadcaster * broadcaster = parent->GetNodeContext()->GetParent()->GetSimulationEventContext()->GetNodeEventBroadcaster();
        release_assert( broadcaster );
        for (auto &trigger : m_trigger_conditions)
        {
            LOG_DEBUG_F( "Unregistering as observer of event %s.\n", trigger.c_str() );
            broadcaster->UnregisterObserver( (INodeEventObserver*)this, trigger );
        }
        SetExpired( true );
    }

    void NLHTIVNode::Update(float dt)
    {
        LOG_DEBUG_F( "Update\n" );
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
        event_occured_list.resize( EventTriggerNodeFactory::GetInstance()->GetNumEventTriggers() );

        blackout_time_remaining -= dt ;
        if( notification_occured )
        {
            notification_occured = false ;
            blackout_time_remaining = blackout_period ;
            LOG_DEBUG_F( "start blackout period - nodeid=%d\n",parent->GetExternalId() );
        }
    }

    void NLHTIVNode::SetContextTo(INodeEventContext *context)
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
        if( _ndi == nullptr )
        {
            Configuration* config = nullptr;
            config = Configuration::CopyFromElement( (actual_node_intervention_config._json), "campaign" );

            _ndi = const_cast<IInterventionFactory*>(ifobj)->CreateNDIIntervention( config );
            release_assert( _ndi != nullptr );

            delete config;
            config = nullptr;
        }
    }

#if 0
    // private/protected
    bool
    NLHTIVNode::qualifiesToGetIntervention(
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

    float NLHTIVNode::getDemographicCoverage() const
    {
        return demographic_restrictions.GetDemographicCoverage();    
    }

    void NLHTIVNode::onDisqualifiedByCoverage( IIndividualHumanEventContext *pIndiv )
    {
        //do nothing, this is for the scale up switch
    }
#endif

    std::string NLHTIVNode::GetInterventionClassName() const
    {
        std::string class_name;
        class_name = std::string( json::QuickInterpreter( actual_node_intervention_config._json )[ "class" ].As<json::String>() );
        return class_name;
    }
}
