/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVDelayedIntervention.h"
#include "MathFunctions.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "NodeEventContext.h"
#include "RANDOM.h"
#include "IHIVInterventionsContainer.h" // for time-date util function
#include "IdmDateTime.h"
#include "DistributionFactory.h"
#include "InterpolatedValueMap.h"

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

        DistributionFunction::Enum delay_function( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Delay_Period_Distribution", delay_function, inputJson, MetadataDescriptor::Enum( "Delay_Distribution", DI_Delay_Distribution_DESC_TEXT, MDD_ENUM_ARGS( DistributionFunction ) ) );
        delay_distribution = DistributionFactory::CreateDistribution( this, delay_function, "Delay_Period", inputJson );

        initConfigTypeMap( "Broadcast_Event", &broadcast_event, HIV_Delayed_Intervention_Broadcast_Event_DESC_TEXT );
        initConfigTypeMap( "Broadcast_On_Expiration_Event", &broadcast_on_expiration_event, HIV_Delayed_Intervention_Broadcast_On_Expiration_Event_DESC_TEXT );

        // skip DelayedIntervention::Configure() because we don't want those variables.

        bool ret = BaseIntervention::Configure(inputJson);
        if ( ret && !JsonConfigurable::_dryrun )
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
        if ( delay_distribution->GetIPiecewiseDistribution() )
        {
            auto year = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
            delay_distribution->GetIPiecewiseDistribution()->SetX( year );
            remaining_delay_days = delay_distribution->Calculate( parent->GetRng() );
            //LOG_DEBUG_F( "Selecting (for now) %f as delay days because map year %d is not > current year %d\n", remaining_delay_days, map_year, (int) current_year );
            LOG_DEBUG_F( "Selecting (for now) %f as delay days.\n", float( remaining_delay_days ) );
        }
        else
        {
            DelayedIntervention::CalculateDelay();
        }
        LOG_DEBUG_F("Drew %0.2f remaining delay days in %s.\n", float(remaining_delay_days), DistributionFunction::pairs::lookup_key(delay_distribution->GetType()));
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

            if( !broadcast_on_expiration_event.IsUninitialized() )
            {
                IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
                broadcaster->TriggerObservers( parent->GetEventContext(), broadcast_on_expiration_event );
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
            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( parent->GetEventContext(), broadcast_event );
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
