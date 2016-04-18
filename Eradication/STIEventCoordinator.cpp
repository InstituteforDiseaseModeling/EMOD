/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <sstream>
#include "Debug.h"
#include "Sugar.h"
#include "Environment.h"
#include "STIEventCoordinator.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"
#include "FactorySupport.h"
#include "InterventionFactory.h"
#include "IIndividualHuman.h"
#include "NodeEventContext.h"
#include "Log.h"
#include "IIndividualHumanSTI.h"
#include "SimulationConfig.h"

static const char * _module = "STIEventCoordinator";


// Let's have some fun and customize this. Log out which nodes get the intervention, and which inviduals, track
// them in a map, make bednet's conditional on some node info we get, and some individual info (not just coverage/
// randomness, then lets' repeat in 30 days 1 time.
// Then test out taking bednets away from migrators (just as a test).
namespace Kernel
{

    IMPLEMENT_FACTORY_REGISTERED(STIInterventionDistributionEventCoordinator)

    IMPL_QUERY_INTERFACE2(STIInterventionDistributionEventCoordinator, IEventCoordinator, IConfigurable)

    QuickBuilder
    STIInterventionDistributionEventCoordinator::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    // ctor
    STIInterventionDistributionEventCoordinator::STIInterventionDistributionEventCoordinator()
    {
        LOG_DEBUG("STIInterventionDistributionEventCoordinator ctor\n");
/*
        initConfigTypeMap("Number_Distributions", &num_distributions, Number_Distributions_DESC_TEXT, -1, INT_MAX, -1 ); // by convention, -1 means no limit
        initConfigTypeMap("Demographic_Coverage", &demographic_coverage, Demographic_Coverage_DESC_TEXT, 0.0, 1.0, 1.0, "Intervention_Config.*.iv_type", "IndividualTargeted" );

        initConfigTypeMap("Number_Repetitions", &num_repetitions, Number_Repetitions_DESC_TEXT, -1, 1000, -1 );
        initConfigTypeMap("Travel_Linked", &travel_linked, Travel_Linked_DESC_TEXT, 0, 1, 0, "Intervention_Config.*.iv_type", "IndividualTargeted" );
        initConfigTypeMap("Include_Departures", &include_emigrants, Include_Departures_DESC_TEXT, 0, 1, 0, "Travel_Linked", "true" );
        initConfigTypeMap("Include_Arrivals", &include_immigrants, Include_Arrivals_DESC_TEXT, 0, 1, 0, "Travel_Linked", "true" );
*/
    }

    bool
    STIInterventionDistributionEventCoordinator::Configure(
        const Configuration * inputJson
    )
    {
        initConfig("BSS_Targeting", bss_targeting, inputJson, MetadataDescriptor::Enum("BSS_Targeting", BSS_TARGETING_DESC_TEXT, MDD_ENUM_ARGS(BssTargetingType)), "Intervention_Config.*.iv_type", "IndividualTargeted");
        LOG_DEBUG_F( "bss_targeting configured as %s\n", BssTargetingType::pairs::lookup_key( bss_targeting ) );

        JsonConfigurable::_useDefaults = InterventionFactory::useDefaults;
        /*
        initConfigTypeMap("Intervention_Config", &intervention_config, Intervention_Config_DESC_TEXT );
        initConfig( "Target_Demographic", target_demographic, inputJson, MetadataDescriptor::Enum("target_demographic", Target_Demographic_DESC_TEXT, MDD_ENUM_ARGS(TargetDemographicType)), "Intervention_Config.*.iv_type", "IndividualTargeted");
        if ( target_demographic == TargetDemographicType::ExplicitAgeRanges || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Target_Age_Min", &target_age_min, Target_Age_Min_DESC_TEXT, 0.0f, 125.0f, 0.0f, "Target_Demographic", "ExplicitAgeRanges" );
            initConfigTypeMap( "Target_Age_Max", &target_age_max, Target_Age_Max_DESC_TEXT, 0.0f, 125.0f, 125.0f, "Target_Demographic", "ExplicitAgeRanges" );
        }
        initConfigTypeMap("Property_Restrictions", &property_restrictions, Property_Restriction_DESC_TEXT, "Intervention_Config.*.iv_type", "IndividualTargeted" );
        */
        bool retValue = StandardInterventionDistributionEventCoordinator::Configure( inputJson );
        JsonConfigurable::_useDefaults = false;
        return retValue;
    }

    // private/protected
    float
    STIInterventionDistributionEventCoordinator::getDemographicCoverageForIndividual(
        const IIndividualHumanEventContext * const pIndividual
    )
    const
    {
        // bss_targeting can either be TargetBss, IgnoreBss, or NeutralBss.
        // If TargetBss, then coverage is 1.0-ish for BSS individuals.
        // If IgnoreBss, then coverage is 0.0 for BSS individuals.
        // If NeutralBss, then coverage is regular value for BSS individuals.
        // Some slightly interesting math here that I should probably document once I confirm this isn't overkill.
        // Givens: 
        //   N: population
        //   Ps: Probability person is BSS aka Percentage of Pop is BSS (0.2 = 20% BSS)
        //   C: intended demographic coverage for entire pop
        //   Nbss: BSS Population
        //   Nreg: Non-BSS Population
        // Calculate:
        //   Pbss: Adjusted coverage for BSS sub-population.
        //   Preg: Adjusted coverage for non-BSS sub-population.
        // Case 0: (Ps >= C) aka there are enough BSS to achieve target coverage entirely in that subpopulation
        //   Preg = 0
        //   Pbss = C/Ps
        // Case 1: (Ps < C) aka there are not enough BSS to achieve target coverage, so need some from regular sub-pop
        //   Pbss = 1.0
        //   Preg = ( C-Ps )/( 1-Ps )
        float p_iss = GET_CONFIGURABLE(SimulationConfig)->prob_super_spreader;
        float base_coverage = StandardInterventionDistributionEventCoordinator::getDemographicCoverageForIndividual( pIndividual );
        float ret = 1.0f;

        const auto * pStiIndividual = dynamic_cast<const IIndividualHumanSTI*>(pIndividual);
        if( pStiIndividual->IsBehavioralSuperSpreader() )
        {
            if( bss_targeting == BssTargetingType::TargetBss )
            {
                float retCvgForTargetedBss = 1.0f;
                if( p_iss >= base_coverage )
                {
                    // potentially odd situation where our probability of being a BSS is higher than our targeted (outbreak?) coverage
                    retCvgForTargetedBss = base_coverage / p_iss;
                }

                LOG_DEBUG_F( "Returning %f for coverage because individual is BSS and we are targeting BSS.\n", retCvgForTargetedBss );
                ret = retCvgForTargetedBss;
            }
            else if( bss_targeting == BssTargetingType::IgnoreBss )
            {
                LOG_DEBUG_F( "Returning 0.0 for coverage because individual is BSS and we are ignoring BSS.\n" );
                ret = 0.0f;
            }
            else // we are BSS-neutral
            {
                ret = base_coverage;
            }
        }
        else
        {
            float retCvg = 0.0f;
            if( bss_targeting == BssTargetingType::TargetBss )
            {
                if( p_iss < base_coverage )
                {
                    retCvg = (base_coverage-p_iss) / (1-p_iss);
                }
                LOG_DEBUG_F( "Returning coverage of %f  for non-BSS individuals for intervention targeted at BSS.\n", retCvg );
                ret = retCvg;
            }
            else if( bss_targeting == BssTargetingType::IgnoreBss )
            {
                float factor = 1/(1-p_iss);
                LOG_DEBUG_F( "Multiplying regular (non-BSS) coverage by %f.\n", factor );
                ret = factor * getDemographicCoverageForIndividual( pIndividual );
            }
            else // we are BSS-neutral
            {
                ret = base_coverage;
            }
        }
        return ret;
    }
}
