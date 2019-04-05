/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReferenceTrackingEventCoordinator.h"
#include "SimulationConfig.h"
#include "Simulation.h"
#include "SimulationEventContext.h"
#include <algorithm> // for std:find

SETUP_LOGGING( "ReferenceTrackingEventCoordinator" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(ReferenceTrackingEventCoordinator)
    IMPL_QUERY_INTERFACE2(ReferenceTrackingEventCoordinator, IEventCoordinator, IConfigurable)

    ReferenceTrackingEventCoordinator::ReferenceTrackingEventCoordinator()
        : StandardInterventionDistributionEventCoordinator( false )//false=don't use standard demographic coverage
        , year2ValueMap()
        , end_year(0.0)
    {
    }

    bool
    ReferenceTrackingEventCoordinator::Configure(
        const Configuration * inputJson
    )
    {
        if( !JsonConfigurable::_dryrun &&
#ifdef ENABLE_TYPHOID
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::TYPHOID_SIM) &&
#endif
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::STI_SIM) &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::HIV_SIM) )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "ReferenceTrackingEventCoordinator can only be used in STI, HIV, and TYPHOID simulations." );
        }

        float update_period = DAYSPERYEAR;
        initConfigComplexType("Time_Value_Map", &year2ValueMap, RTEC_Time_Value_Map_DESC_TEXT );
        initConfigTypeMap(    "Update_Period",  &update_period, RTEC_Update_Period_DESC_TEXT, 1.0,      10*DAYSPERYEAR, DAYSPERYEAR );
        initConfigTypeMap(    "End_Year",       &end_year,      RTEC_End_Year_DESC_TEXT,      MIN_YEAR, MAX_YEAR,       MAX_YEAR );

        auto ret = StandardInterventionDistributionEventCoordinator::Configure( inputJson );
        num_repetitions = -1; // unlimited
        if( JsonConfigurable::_dryrun == false )
        {
            float dt = GET_CONFIGURABLE(SimulationConfig)->Sim_Tstep;
            tsteps_between_reps = update_period/dt; // this won't be precise, depending on math.
            if( tsteps_between_reps <= 0.0 )
            {
                // don't let this be zero or it will only update one time
                tsteps_between_reps = 1;
            }
        }
        LOG_DEBUG_F( "ReferenceTrackingEventCoordinator configured with update_period = %f, end_year = %f, and tsteps_between_reps (derived) = %d.\n", update_period, end_year, tsteps_between_reps );
        return ret;
    }

    void ReferenceTrackingEventCoordinator::CheckStartDay( float campaignStartDay ) const
    {
        float campaign_start_year = campaignStartDay / DAYSPERYEAR + Simulation::base_year;
        if( end_year <= campaign_start_year )
        {
            LOG_WARN_F( "Campaign starts on year %f (day=%f). A ReferenceTrackingEventCoordinator ends on End_Year %f.  It will not distribute any interventions.\n",
                        campaign_start_year, campaignStartDay, end_year );
        }

        if( campaign_start_year != year2ValueMap.begin()->first )
        {
            LOG_WARN_F( "Campaign starts on year %f (day=%f). A ReferenceTrackingEventCoordinator has a Time_Value_Map that starts on year %f.\n",
                        campaign_start_year, campaignStartDay, year2ValueMap.begin()->first );
        }
    }

    void ReferenceTrackingEventCoordinator::InitializeRepetitions( const Configuration* inputJson )
    {
        // don't include repetition parameters since they are managed internally by this class
    }

    // Obviously don't need this if it's not doing anything useful.
    void ReferenceTrackingEventCoordinator::Update( float dt )
    {
        // Check if it's time for another distribution
        if( parent->GetSimulationTime().Year() >= end_year )
        {
            LOG_INFO_F( "ReferenceTrackingEventCoordinator expired.\n" );
            distribution_complete = true;
        }
        LOG_DEBUG_F( "Update...ing.\n" );
        return StandardInterventionDistributionEventCoordinator::Update( dt );
    }

    // The purpose of this function is to calculate the existing coverage of the intervention in question
    // and then to set the target coverage based on the error between measured and configured (for current time).
    void ReferenceTrackingEventCoordinator::preDistribute()
    {
        LOG_DEBUG_F( "preDistributed.\n" );
        // Two variables that will be used by lambda function that's called for each individual;
        // these vars accumulate values across the population. 
        NonNegativeFloat totalWithIntervention = 0.0f;
        NonNegativeFloat totalQualifyingPop = 0.0f;
        haves.clear();

        // This is the function that will be called for each individual in this node (event_context)
        INodeEventContext::individual_visit_function_t fn = 
            [ this, &totalWithIntervention, &totalQualifyingPop ](IIndividualHumanEventContext *ihec)
        {
            if( qualifiesDemographically( ihec ) )
            {
                auto mcw = ihec->GetMonteCarloWeight();
                totalQualifyingPop += mcw;

                // Check whether this individual has this intervention
                auto better_ptr = ihec->GetInterventionsContext();
                std::string intervention_name = _di->GetName();
                if( better_ptr->ContainsExistingByName( intervention_name ) )
                {
                    totalWithIntervention += mcw;
                    haves.push_back( ihec->GetSuid().data );
                }
            }
        };

        // foreach node...
        for (auto event_context : cached_nodes)
        {
            event_context->VisitIndividuals( fn ); // does not return value, updates total existing coverage by capture
        }

        float dc = 0.0f;
        if( totalQualifyingPop > 0 )
        {
            Fraction currentCoverageForIntervention = totalWithIntervention/totalQualifyingPop;
            NonNegativeFloat totalWithoutIntervention = totalQualifyingPop - totalWithIntervention;
            float default_value = 0.0f;
            float year = parent->GetSimulationTime().Year();
            float target_coverage  = year2ValueMap.getValueLinearInterpolation(year, default_value);

            float totalToIntervene = ( target_coverage * totalQualifyingPop ) - totalWithIntervention;
            NO_LESS_THAN( totalToIntervene, 0 );

            if( totalWithoutIntervention > 0 )
            {
                dc = totalToIntervene / totalWithoutIntervention;
            }
            LOG_INFO_F( "Setting demographic_coverage to %f based on target_coverage = %f, currentCoverageForIntervention = %f, total without intervention  = %f, total with intervention = %f.\n",
                            dc,
                            float(target_coverage),
                            float(currentCoverageForIntervention),
                            float(totalWithoutIntervention),
                            float(totalWithIntervention)
                        );
        }
        else
        {
            LOG_INFO( "Setting demographic_coverage to 0 since 0 qualifying population.\n");
        }
        demographic_restrictions.SetDemographicCoverage( dc );

    }

    bool ReferenceTrackingEventCoordinator::TargetedIndividualIsCovered(IIndividualHumanEventContext *ihec)
    {
        if( std::find( haves.begin(), haves.end(), ihec->GetSuid().data ) != haves.end() )
        {
            // This person already has the intervention. They are in the haves list.
            LOG_DEBUG_F( "Skipping %d coz they already have it.\n", ihec->GetSuid().data );
            return false;
        }
        return StandardInterventionDistributionEventCoordinator::TargetedIndividualIsCovered(ihec);
    }

}

