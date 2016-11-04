/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include <map>

#include "Vector.h"
#include "VectorEnums.h"   // remove dependency when egg, larva, and vector enums are configured outside this class

namespace Kernel
{
    class VectorSpeciesParameters;


    struct VectorParameters
    {
        EggHatchDelayDist::Enum       egg_hatch_delay_dist;             // eggHatchDelayDist
        EggSaturation::Enum           egg_saturation;                   // eggSaturation
        LarvalDensityDependence::Enum larval_density_dependence;        // larvalDensityDependence
        VectorSamplingType::Enum      vector_sampling_type;             // Vector_Sampling_Type
        VectorSugarFeeding::Enum      vector_sugar_feeding;             // vectorSugarFeeding
        VectorRainfallMortality::Enum vector_larval_rainfall_mortality; // vectorLarvalRainfallMortality
        HEGModel::Enum                heg_model;                        //HEGModel

        bool  vector_aging;                     // From SimulationVector
        bool  temperature_dependent_feeding_cycle;
        float meanEggHatchDelay;
        float WolbachiaMortalityModification;
        float WolbachiaInfectionModification;
        float HEGhomingRate;
        float HEGfecundityLimiting;
        float human_feeding_mortality;

        // allows configuration of larval habitat decay time constants
        float tempHabitatDecayScalar;
        float semipermanentHabitatDecayRate;
        float mmRainfallToFillSwamp;
        float larval_rainfall_mortality_threshold;

        // allows configuration of larval density dependence
        float larvalDensityMortalityScalar;
        float larvalDensityMortalityOffset;

        double x_templarvalhabitat;

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
        , vector_aging(false)
        , temperature_dependent_feeding_cycle(false)
        , meanEggHatchDelay(0.0f)
        , WolbachiaMortalityModification(0.0f)
        , WolbachiaInfectionModification(0.0f)
        , HEGhomingRate(0.0f)
        , HEGfecundityLimiting(0.0f)
        , human_feeding_mortality(DEFAULT_HUMAN_FEEDING_MORTALITY)
        , tempHabitatDecayScalar(0.0f)
        , semipermanentHabitatDecayRate(0.0f)
        , mmRainfallToFillSwamp(1.0f)
        , larval_rainfall_mortality_threshold(1.0f)
        , larvalDensityMortalityScalar(10.0f)
        , larvalDensityMortalityOffset(0.1f)
        , x_templarvalhabitat(1.0f)
        , vector_species_names()
        , vspMap()
        {
        }
    };
}
