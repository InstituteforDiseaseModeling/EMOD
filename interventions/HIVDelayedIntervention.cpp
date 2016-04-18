/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVDelayedIntervention.h"
#include "MathFunctions.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include "RANDOM.h"
#include "IHIVInterventionsContainer.h" // for time-date util function and access into IHIVCascadeOfCare

static const char * _module = "HIVDelayedIntervention";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVDelayedIntervention, DelayedIntervention)
        HANDLE_INTERFACE(IHIVCascadeStateIntervention)
    END_QUERY_INTERFACE_DERIVED(HIVDelayedIntervention, DelayedIntervention)

    IMPLEMENT_FACTORY_REGISTERED(HIVDelayedIntervention)

    HIVDelayedIntervention::HIVDelayedIntervention()
    : DelayedIntervention()
    , year2DelayMap()
    , abortStates()
    , cascadeState("")
    , days_remaining(-1)
    , firstUpdate(true)
    , broadcast_event()
    , broadcast_on_expiration_event()
    {
        initSimTypes(1, "HIV_SIM");
        delay_distribution.AddSupportedType( DistributionFunction::PIECEWISE_CONSTANT, "", "", "", "" );
        delay_distribution.AddSupportedType( DistributionFunction::PIECEWISE_LINEAR,   "", "", "", "" );
    }

    HIVDelayedIntervention::HIVDelayedIntervention( const HIVDelayedIntervention& master )
        : DelayedIntervention( master )
    {
        year2DelayMap = master.year2DelayMap;
        abortStates = master.abortStates;
        cascadeState = master.cascadeState;
        days_remaining = master.days_remaining;
        firstUpdate = master.firstUpdate;
        broadcast_event = master.broadcast_event;
        broadcast_on_expiration_event = master.broadcast_on_expiration_event;
    }

    bool HIVDelayedIntervention::Configure( const Configuration * inputJson )
    {
        // should be lifted to HIVIntervention class later
        abortStates.value_source = "Valid_Cascade_States.*";
        initConfigTypeMap("Abort_States", &abortStates, HIV_Delayed_Intervention_Abort_States_DESC_TEXT);
        initConfigTypeMap("Cascade_State", &cascadeState, HIV_Delayed_Intervention_Cascade_States_DESC_TEXT);
        initConfigTypeMap("Expiration_Period", &days_remaining, HIV_Delayed_Intervention_Expiration_Period_DESC_TEXT, 0, FLT_MAX, FLT_MAX);

        // DelayedIntervention::Configure split into PreConfigure and MainConfigure to separate initConfig's from initConfigTypeMap-depends-on, and postpone JsonConfigurable::Configure
        DelayedIntervention::PreConfigure(inputJson);
        DelayedIntervention::DistributionConfigure(inputJson);

        // HIVDelayedIntervention specific
        if( delay_distribution.GetType() == DistributionFunction::PIECEWISE_CONSTANT 
            || delay_distribution.GetType() == DistributionFunction::PIECEWISE_LINEAR
            || JsonConfigurable::_dryrun )
        {
            initConfigComplexType( "Time_Varying_Constants",
                                   &year2DelayMap,
                                   HIV_Delayed_Intervention_TVC_DESC_TEXT,
                                   "Delay_Distribution",
                                   "PIECEWISE_CONSTANT || PIECEWISE_LINEAR" );
        }

        //DelayedIntervention::InterventionConfigure(inputJson);
        initConfigTypeMap( "Broadcast_Event", &broadcast_event, HIV_Delayed_Intervention_Broadcast_Event_DESC_TEXT );

        initConfigTypeMap( "Broadcast_On_Expiration_Event", &broadcast_on_expiration_event, HIV_Delayed_Intervention_Broadcast_On_Expiration_Event_DESC_TEXT );

        bool ret = JsonConfigurable::Configure(inputJson);
        if( ret )
        {
            // don't validate the intervention configuration since not supported
            DelayValidate();
            AbortValidate();
        }
        return ret ;
    }

    void HIVDelayedIntervention::AbortValidate()
    {
        // error if the cascadeState is an abortState
        if (abortStates.find(cascadeState) != abortStates.end())
        {
            std::string abort_state_list ;
            for( auto state : abortStates )
            {
                abort_state_list += state + ", " ;
            }
            if( abortStates.size() > 0 )
            {
                abort_state_list = abort_state_list.substr( 0, abort_state_list.length() - 2 );
            }
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                    "Cascade_State", cascadeState.c_str(),
                                                    "Abort_States", abort_state_list.c_str(),
                                                    "The Cascade_State cannot be one of the Abort_States." );
        }
    }

    void
    HIVDelayedIntervention::CalculateDelay()
    {
        switch( delay_distribution.GetType() )
        {
            case DistributionFunction::PIECEWISE_CONSTANT:
            {
                auto year = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
                remaining_delay_days = year2DelayMap.getValuePiecewiseConstant( year );
                //LOG_DEBUG_F( "Selecting (for now) %f as delay days because map year %d is not > current year %d\n", remaining_delay_days, map_year, (int) current_year );
                LOG_DEBUG_F( "Selecting (for now) %f as delay days.\n", remaining_delay_days );
            }
            break;

            case DistributionFunction::PIECEWISE_LINEAR:
            {
                auto year = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
                remaining_delay_days = year2DelayMap.getValueLinearInterpolation( year );
                //LOG_DEBUG_F( "Selecting (for now) %f as delay days because map year %d is not > current year %d\n", remaining_delay_days, map_year, (int) current_year );
                LOG_DEBUG_F( "Selecting (for now) %f as delay days.\n", remaining_delay_days );
            }
            break;

            default:
                DelayedIntervention::CalculateDelay();
            break;
        }
        LOG_DEBUG_F("Drew %0.2f remaining delay days in %s.\n", remaining_delay_days, DistributionFunction::pairs::lookup_key(delay_distribution.GetType()));
    }

    // todo: lift to HIVIntervention or helper function (repeated in HIVSimpleDiagnostic)
    bool HIVDelayedIntervention::UpdateCascade()
    {
        if( AbortDueToCurrentCascadeState() )
        {
            return false ;
        }

        // is this the first time through?  if so, update the cascade state.
        if (firstUpdate)
        {
            IHIVCascadeOfCare * hiv_parent = nullptr;
            if( parent->GetInterventionsContext()->QueryInterface( GET_IID(IHIVCascadeOfCare), (void**)&hiv_parent ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVInterventionsContainer", "IIndividualHumanContext" );
            }

            firstUpdate = false;
            LOG_DEBUG_F( "Setting cascade state to %s for individual %d\n", cascadeState.c_str(), parent->GetSuid().data );
            hiv_parent->setCascadeState(cascadeState);
        }
        return true;
    }

    bool HIVDelayedIntervention::AbortDueToCurrentCascadeState()
    {
        IHIVCascadeOfCare *ihcc = nullptr;
        if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IHIVCascadeOfCare), (void **)&ihcc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent->GetInterventionsContext()",
                                           "IHIVCascadeOfCare",
                                           "IIndividualHumanInterventionsContext" );
        }

        std::string currentState = ihcc->getCascadeState();

        if ( abortStates.find(currentState) != abortStates.end() )
        {

            // Duplicated from SimpleDiagnostic::positiveTestDistribute
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }

            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::CascadeStateAborted );
            LOG_DEBUG_F("The current cascade state \"%s\" is one of the Abort_States.  Expiring the delay for individual %d.\n", currentState.c_str(), parent->GetSuid().data );
            expired = true;

            return true;
        }
        return false;
    }

    bool HIVDelayedIntervention::qualifiesToGetIntervention( IIndividualHumanContext* pIndivid )
    {
        return !AbortDueToCurrentCascadeState();
    }

    bool HIVDelayedIntervention::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pICCO
    )
    {
        parent = context->GetParent();
        LOG_DEBUG_F( "Individual %d is getting a delay.\n", parent->GetSuid().data );

        if( qualifiesToGetIntervention( parent ) )
        {
            return DelayedIntervention::Distribute( context, pICCO );
        }
        else
        {
            expired = true ;
            return false ;
        }
    }


    void HIVDelayedIntervention::Update(float dt)
    {
        bool cascade_state_ok = UpdateCascade();
        if( !cascade_state_ok )
        {
            // the cascadeState must be an abort state
            return ;
        }

        days_remaining -= dt;
        if( days_remaining < 0 )
        {
            expired = true;

            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
            if( (broadcast_on_expiration_event != NO_TRIGGER_STR) && !broadcast_on_expiration_event.IsUninitialized() )
            {
                broadcaster->TriggerNodeEventObserversByString( parent->GetEventContext(), broadcast_on_expiration_event );
            }
            LOG_DEBUG_F("broadcast on expiration event\n");

            LOG_DEBUG_F("expiring before delay-triggered intervention\n");
            return;
        }

        //DelayedIntervention::Update(dt);
        if( remaining_delay_days > 0 )
        {
            remaining_delay_days -= dt;
            return;
        }

        if( expired || (broadcast_event == NO_TRIGGER_STR) || broadcast_event.IsUninitialized() )
        {
            LOG_DEBUG_F("expired or event == NoTrigger\n");
            return;
        }

        // Duplicated from SimpleDiagnostic::positiveTestDistribute
        INodeTriggeredInterventionConsumer* broadcaster = nullptr;
        if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent->GetEventContext()->GetNodeEventContext()",
                                           "INodeTriggeredInterventionConsumer",
                                           "INodeEventContext" );
        }
        broadcaster->TriggerNodeEventObserversByString( parent->GetEventContext(), broadcast_event );
        LOG_DEBUG_F("broadcast actual event\n");

        expired = true;
        return;
    }

    const std::string& HIVDelayedIntervention::GetCascadeState()
    {
        return cascadeState;
    }

    const jsonConfigurable::tDynamicStringSet& HIVDelayedIntervention::GetAbortStates()
    {
        return abortStates;
    }

    REGISTER_SERIALIZABLE(HIVDelayedIntervention);

    void HIVDelayedIntervention::serialize(IArchive& ar, HIVDelayedIntervention* obj)
    {
        DelayedIntervention::serialize( ar, obj );
        HIVDelayedIntervention& delayed = *obj;

        ar.labelElement("year2DelayMap"                 ) & delayed.year2DelayMap;
        ar.labelElement("abortStates"                   ) & delayed.abortStates;
        ar.labelElement("cascadeState"                  ) & delayed.cascadeState;
        ar.labelElement("days_remaining"                ) & delayed.days_remaining;
        ar.labelElement("firstUpdate"                   ) & delayed.firstUpdate;
        ar.labelElement("broadcast_event"               ) & delayed.broadcast_event;
        ar.labelElement("broadcast_on_expiration_event" ) & delayed.broadcast_on_expiration_event;
    }
}
