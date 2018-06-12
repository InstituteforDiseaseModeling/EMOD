/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVDelayedIntervention.h"
#include "MathFunctions.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include "RANDOM.h"
#include "IHIVInterventionsContainer.h" // for time-date util function

SETUP_LOGGING( "HIVDelayedIntervention" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVDelayedIntervention, DelayedIntervention)
    END_QUERY_INTERFACE_DERIVED(HIVDelayedIntervention, DelayedIntervention)

    IMPLEMENT_FACTORY_REGISTERED(HIVDelayedIntervention)

    HIVDelayedIntervention::HIVDelayedIntervention()
    : DelayedIntervention()
    , year2DelayMap()
    , days_remaining(-1)
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
        days_remaining = master.days_remaining;
        broadcast_event = master.broadcast_event;
        broadcast_on_expiration_event = master.broadcast_on_expiration_event;
    }

    bool HIVDelayedIntervention::Configure( const Configuration * inputJson )
    {
        // should be lifted to HIVIntervention class later
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

        // skip DelayedIntervention::Configure() because we don't want those variables.
        bool ret = BaseIntervention::Configure(inputJson);
        if( ret )
        {
            // don't validate the intervention configuration since not supported
            DelayValidate();
        }

        LOG_DEBUG_F( "HIVDelayedIntervention configured with days_remaining = %f and remaining_delay_days = %f\n", (float) days_remaining, (float) remaining_delay_days );
        return ret ;
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
                LOG_DEBUG_F( "Selecting (for now) %f as delay days.\n", float(remaining_delay_days) );
            }
            break;

            case DistributionFunction::PIECEWISE_LINEAR:
            {
                auto year = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
                remaining_delay_days = year2DelayMap.getValueLinearInterpolation( year );
                //LOG_DEBUG_F( "Selecting (for now) %f as delay days because map year %d is not > current year %d\n", remaining_delay_days, map_year, (int) current_year );
                LOG_DEBUG_F( "Selecting (for now) %f as delay days.\n", float(remaining_delay_days) );
            }
            break;

            default:
                DelayedIntervention::CalculateDelay();
            break;
        }
        LOG_DEBUG_F("Drew %0.2f remaining delay days in %s.\n", float(remaining_delay_days), DistributionFunction::pairs::lookup_key(delay_distribution.GetType()));
    }

    void HIVDelayedIntervention::Update(float dt)
    {
        if( !DelayedIntervention::UpdateIndividualsInterventionStatus() )
        {
            return;
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
            if( !broadcast_on_expiration_event.IsUninitialized() )
            {
                broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), broadcast_on_expiration_event );
            }
            LOG_DEBUG_F("broadcast on expiration event\n");

            LOG_DEBUG_F("expiring before delay-triggered intervention\n");
            return;
        }

        remaining_delay_days.Decrement( dt );
    }

    void HIVDelayedIntervention::Callback( float dt )
    {
        if( expired || broadcast_event.IsUninitialized() )
        {
            LOG_DEBUG_F("expired or event is unitialized\n");
        }
        else
        {
            // Duplicated from SimpleDiagnostic::positiveTestDistribute
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "parent->GetEventContext()->GetNodeEventContext()",
                                               "INodeTriggeredInterventionConsumer",
                                               "INodeEventContext" );
            }
            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), broadcast_event );
            LOG_DEBUG_F("broadcast actual event\n");
        }
        expired = true;
    }

    REGISTER_SERIALIZABLE(HIVDelayedIntervention);

    void HIVDelayedIntervention::serialize(IArchive& ar, HIVDelayedIntervention* obj)
    {
        DelayedIntervention::serialize( ar, obj );
        HIVDelayedIntervention& delayed = *obj;

        ar.labelElement("year2DelayMap"                 ) & delayed.year2DelayMap;
        ar.labelElement("days_remaining"                ) & delayed.days_remaining;
        ar.labelElement("broadcast_event"               ) & delayed.broadcast_event;
        ar.labelElement("broadcast_on_expiration_event" ) & delayed.broadcast_on_expiration_event;
    }
}
