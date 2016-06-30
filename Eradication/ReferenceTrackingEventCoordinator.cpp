/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReferenceTrackingEventCoordinator.h"
#include "SimulationConfig.h"

static const char * _module = "ReferenceTrackingEventCoordinator";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(ReferenceTrackingEventCoordinator)
    IMPL_QUERY_INTERFACE2(ReferenceTrackingEventCoordinator, IEventCoordinator, IConfigurable)

    ReferenceTrackingEventCoordinator::ReferenceTrackingEventCoordinator()
    : year2ValueMap()
    , target_coverage( 0.0 ) // no great reason for this value
    , end_year(0.0)
    {
    }

    QuickBuilder
    ReferenceTrackingEventCoordinator::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    bool
    ReferenceTrackingEventCoordinator::Configure(
        const Configuration * inputJson
    )
    {
        if( !JsonConfigurable::_dryrun &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::STI_SIM) &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::HIV_SIM) )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "ReferenceTrackingEventCoordinator can only be used in STI and HIV simulations." );
        }

        float update_period = DAYSPERYEAR;
        initConfigComplexType("Time_Value_Map", &year2ValueMap, "Map of times (years) to coverages." );
        initConfigTypeMap("Update_Period", &update_period, "Gap between distribution updates.", 1.0, 10*DAYSPERYEAR, DAYSPERYEAR );
        initConfigTypeMap("End_Year", &end_year, "Final date at which this set of targeted coverages should be applied (expiration)", MIN_YEAR, MAX_YEAR, MAX_YEAR );

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
        return ret;
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
        return StandardInterventionDistributionEventCoordinator::Update( dt );
    }

    // The purpose of this function is to calculate the existing coverage of the intervention in question
    // and then to set the target coverage based on the error between measured and configured (for current time).
    void ReferenceTrackingEventCoordinator::preDistribute()
    {
        // Two variables that will be used by lambda function that's called for each individual;
        // these vars accumulate values across the population. 
        NonNegativeFloat totalWithIntervention = 0.0f;
        NonNegativeFloat totalQualifyingPop = 0.0f;

        // This is the function that will be called for each individual in this node (event_context)
        INodeEventContext::individual_visit_function_t fn = 
            [ this, &totalWithIntervention, &totalQualifyingPop ](IIndividualHumanEventContext *ihec)
        {
            if( qualifiesDemographically( ihec ) )
            {
                auto mcw = ihec->GetMonteCarloWeight();
                totalQualifyingPop += mcw;
                auto better_ptr = ihec->GetInterventionsContext();
                // Check whether this individual has a non-zero quantity of this intervention
                std::string intervention_name = _di->GetName();
                totalWithIntervention += ( (better_ptr->GetInterventionsByName( intervention_name ).size() > 0) ? mcw : 0 );
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
            target_coverage  = year2ValueMap.getValueLinearInterpolation(year, default_value);

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

}

