/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include <map>

#include "Vector.h"
#include "VectorEnums.h"   // remove dependency when egg, larva, and vector enums are configured outside this class
#include "Configure.h"

namespace Kernel
{
    class VectorSpeciesParameters;


    struct VectorParameters
    {
        EggHatchDelayDist::Enum                 egg_hatch_delay_dist;                // eggHatchDelayDist
        EggSaturation::Enum                     egg_saturation;                      // eggSaturation
        LarvalDensityDependence::Enum           larval_density_dependence;           // larvalDensityDependence
        VectorSamplingType::Enum                vector_sampling_type;                // Vector_Sampling_Type
        VectorSugarFeeding::Enum                vector_sugar_feeding;                // vectorSugarFeeding
        VectorRainfallMortality::Enum           vector_larval_rainfall_mortality;    // vectorLarvalRainfallMortality
        HEGModel::Enum                          heg_model;                           // HEGModel
        TemperatureDependentFeedingCycle::Enum  temperature_dependent_feeding_cycle; // Temperature_Dependent_Feeding_Cycle
        EggHatchDensityDependence::Enum         egg_hatch_density_dependence;        // Egg_Hatch_Density_Dependence 

        bool  vector_aging;
        bool  temperature_dependent_egg_hatching;
        bool  egg_mortality; 
        bool  delayed_hatching_when_habitat_dries_up;
        float meanEggHatchDelay;
        float droughtEggHatchDelay;
        float WolbachiaMortalityModification;
        float WolbachiaInfectionModification;
        float HEGhomingRate;
        float HEGfecundityLimiting;
        float human_feeding_mortality;
        float eggarrhenius1;
        float eggarrhenius2;

        // allows configuration of larval habitat decay time constants
        float tempHabitatDecayScalar;
        float semipermanentHabitatDecayRate;
        float mmRainfallToFillSwamp;
        float larval_rainfall_mortality_threshold;

        // allows configuration of larval density dependence
        float larvalDensityMortalityScalar;
        float larvalDensityMortalityOffset;

        float x_tempLarvalHabitat;

        jsonConfigurable::tDynamicStringSet vector_species_names;
        std::map< std::string, VectorSpeciesParameters * > vspMap;


        VectorParameters()
        : egg_hatch_delay_dist(EggHatchDelayDist::NO_DELAY)
        , egg_saturation(EggSaturation::NO_SATURATION)
        , larval_density_dependence(LarvalDensityDependence::NO_DENSITY_DEPENDENCE)
        , vector_sampling_type(VectorSamplingType::TRACK_ALL_VECTORS)
        , vector_sugar_feeding(VectorSugarFeeding::VECTOR_SUGAR_FEEDING_NONE)
        , vector_larval_rainfall_mortality(VectorRainfallMortality::NONE)
        , heg_model(HEGModel::OFF)
        , temperature_dependent_feeding_cycle(TemperatureDependentFeedingCycle::NO_TEMPERATURE_DEPENDENCE)
        , egg_hatch_density_dependence( EggHatchDensityDependence::NO_DENSITY_DEPENDENCE )
        , vector_aging(false)
        , temperature_dependent_egg_hatching(false)
        , egg_mortality(false)
        , delayed_hatching_when_habitat_dries_up(false)
        , meanEggHatchDelay(0.0f)
        , droughtEggHatchDelay(0.0f)
        , WolbachiaMortalityModification(0.0f)
        , WolbachiaInfectionModification(0.0f)
        , HEGhomingRate(0.0f)
        , HEGfecundityLimiting(0.0f)
        , human_feeding_mortality(DEFAULT_HUMAN_FEEDING_MORTALITY)
        , eggarrhenius1( 0.0f) 
        , eggarrhenius2( 0.0f) 
        , tempHabitatDecayScalar(0.0f)
        , semipermanentHabitatDecayRate(0.0f)
        , mmRainfallToFillSwamp(1.0f)
        , larval_rainfall_mortality_threshold(1.0f)
        , larvalDensityMortalityScalar(10.0f)
        , larvalDensityMortalityOffset(0.1f)
        , x_tempLarvalHabitat(1.0f)
        , vector_species_names()
        , vspMap()
        {
        }
    };
}
