/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "VectorHabitat.h"

#include "Climate.h"
#include "Contexts.h"
#include "Exceptions.h"
#include "Interventions.h"
#include "Log.h"
#include "NodeVector.h"
#include "SimulationConfig.h" // TODO: long-term, it seems that lloffset should belong to Node and the larval decay rates and rainfall mortality thresholds to this class.
#include "Vector.h"

static const char * _module = "VectorHabitat";

namespace Kernel
{
    VectorHabitat::VectorHabitat( VectorHabitatType::Enum type, float max_capacity )
        : m_habitat_type(type)
        , m_max_larval_capacity(max_capacity)
        , m_current_larval_capacity(0.0f)
        , m_total_larva_count(2) // current timestep and previous.
        , m_new_egg_count(0)
        , m_oviposition_trap_killing(0.0f)
        , m_artificial_larval_mortality(0.0f)
        , m_larvicide_habitat_scaling(1.0f)
        , m_rainfall_mortality(0.0f)
        , m_egg_crowding_correction(0.0f)
    {
    }

    VectorHabitat::~VectorHabitat()
    {
    }

    VectorHabitat* VectorHabitat::CreateHabitat( VectorHabitatType::Enum type, float max_capacity )
    {
        return new VectorHabitat(type, max_capacity);
    }

    void VectorHabitat::Update(float dt, INodeContext* node)
    {
        // (1) Calculate the egg-crowding correction based on eggs, larva, and habitat from previous step
        CalculateEggCrowdingCorrection();

        // (2) Update the current larval capacity based on local climate and maximum capacity
        UpdateCurrentLarvalCapacity(dt, node);

        // (3) Update the larval-habitat-specific probabilities related to node-targeted interventions
        UpdateLarvalProbabilities(dt, node);

        // (4) Calculate rainfall mortality
        float rainfall = node->GetLocalWeather()->accumulated_rainfall();
        UpdateRainfallMortality(dt, rainfall);

        // (5) Advance total larval counts by a timestep
        AdvanceTotalLarvaCounts();
    }

    void VectorHabitat::CalculateEggCrowdingCorrection()
    {
        float larvalhabitat = GetCurrentLarvalCapacity();
        float total_larva_count = m_total_larva_count[CURRENT_TIME_STEP]; 

        if (larvalhabitat > total_larva_count && m_new_egg_count > 0  && params()->egg_saturation) 
        {
            // Eggs divided up by habitat, neweggs holds number of female eggs.  Correcting for overcrowding, applies to all eggs evenly
            // Will hatch equal number of male larva, but these don't take up larval habitat in the calculations, to keep compatible with older results
            if (m_new_egg_count > larvalhabitat - total_larva_count)
            { 
                m_egg_crowding_correction = float(larvalhabitat - total_larva_count) / float(m_new_egg_count);
            }
            else
            {
                m_egg_crowding_correction = 1.0;
            }
        }
        else if (params()->egg_saturation == EggSaturation::NO_SATURATION)
        {
            m_egg_crowding_correction = 1.0;
        }
        else if (params()->egg_saturation == EggSaturation::SIGMOIDAL_SATURATION)
        {
            if(larvalhabitat > 0){m_egg_crowding_correction = exp(-total_larva_count/larvalhabitat);}
            else{ m_egg_crowding_correction = 0.0;}
        }
        else
        {
            // Habitat is full.  No new eggs.
            m_egg_crowding_correction = 0;
        }

        // After egg-crowding calculation has been calculated, reset new egg counter
        m_new_egg_count = 0;
    }

   void VectorHabitat::UpdateCurrentLarvalCapacity(float dt, INodeContext* node)
    {
        // TODO: potentially we could differentiate the various update behaviors
        //       by using derived classes, e.g. TemporaryRainfallHabitat, WaterVegetationHabitat

        const Climate* localWeather = node->GetLocalWeather();

        switch ( m_habitat_type )
        {
            case VectorHabitatType::CONSTANT:
                LOG_DEBUG_F("Habitat type = CONSTANT, Max larval capacity = %f\n", m_max_larval_capacity);
                m_current_larval_capacity = m_max_larval_capacity * params()->lloffset * params()->lloffset;
                break;

            case VectorHabitatType::TEMPORARY_RAINFALL:
                LOG_DEBUG_F("Habitat type = TEMPORARY_RAINFALL, Max larval capacity = %f\n", m_max_larval_capacity);
                m_current_larval_capacity += (float)( localWeather->accumulated_rainfall() * m_max_larval_capacity * params()->lloffset * params()->lloffset - m_current_larval_capacity * exp(LARVAL_HABITAT_FACTOR1 / (localWeather->airtemperature() + CELSIUS_TO_KELVIN)) * LARVAL_HABITAT_FACTOR2 * params()->tempHabitatDecayScalar * sqrt(LARVAL_HABITAT_FACTOR3 / (localWeather->airtemperature() + CELSIUS_TO_KELVIN)) * (1.0f - localWeather->humidity()) * dt );
                break;

            case VectorHabitatType::WATER_VEGETATION:
                LOG_DEBUG_F("Habitat type = WATER_VEGETATION, Max larval capacity = %f\n", m_max_larval_capacity);
                m_current_larval_capacity += localWeather->accumulated_rainfall() * m_max_larval_capacity * params()->lloffset * params()->lloffset - params()->semipermanentHabitatDecayRate * dt * m_current_larval_capacity;
                break;

            case VectorHabitatType::HUMAN_POPULATION:
                LOG_DEBUG_F("Habitat type = HUMAN_POPULATION, Max larval capacity = %f\n", m_max_larval_capacity);
                m_current_larval_capacity = m_max_larval_capacity * node->GetStatPop();
                break;

            case VectorHabitatType::BRACKISH_SWAMP: 
                LOG_DEBUG_F("Habitat type = BRACKISH_SWAMP, Max larval capacity = %f\n", m_max_larval_capacity);
                m_current_larval_capacity += localWeather->accumulated_rainfall() * (float)MM_PER_METER / params()->mmRainfallToFillSwamp * m_max_larval_capacity * params()->lloffset * params()->lloffset - params()->semipermanentHabitatDecayRate * dt * m_current_larval_capacity;
                if(m_current_larval_capacity > m_max_larval_capacity * params()->lloffset * params()->lloffset)
                {
                    m_current_larval_capacity = m_max_larval_capacity * params()->lloffset * params()->lloffset;
                }
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "m_habitat_type", m_habitat_type, VectorHabitatType::pairs::lookup_key(m_habitat_type) );
        }
    }

    void VectorHabitat::UpdateLarvalProbabilities(float dt, INodeContext* node)
    {
        // TODO: if this querying gets tedious, we can pass it in as an argument from NodeVector?
        INodeVectorInterventionEffects* invie = NULL;
        if (s_OK != node->GetEventContext()->QueryInterface(GET_IID(INodeVectorInterventionEffects), (void**)&invie))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "GetEventContext()", "INodeVectorInterventionEffects", "INodeEventContext" );
        }

        m_oviposition_trap_killing    = invie->GetOviTrapKilling(m_habitat_type);
        m_artificial_larval_mortality = EXPCDF( -dt * ( invie->GetLarvalKilling(m_habitat_type) ) );
        m_larvicide_habitat_scaling   = 1.0f - invie->GetLarvalHabitatReduction(m_habitat_type);

        LOG_DEBUG_F("Updated larval probabilites: oviposition-trap killing = %f, artificial larval mortality = %f, larvicide habitat reduction = %f\n", m_oviposition_trap_killing, m_artificial_larval_mortality, m_larvicide_habitat_scaling);
    }

    void VectorHabitat::UpdateRainfallMortality(float dt, float rainfall)
    {
        // TODO: this calculation is only habitat dependent for the case of VectorRainfallMortality::SIGMOID_HABITAT_SHIFTING
        //       that was designed to make "flushing" of BRACKISH_SWAMP habitat more likely if the habitat were almost full.
        //       in practice, this makes little difference for the Solomon Island rainfall patterns.

        // Paaijmans, K. P., M. O. Wandago, et al. (2007). "Unexpected High Losses of Anopheles gambiae Larvae Due to Rainfall." PLoS One 2(11): e1146.
        if(params()->vector_larval_rainfall_mortality == VectorRainfallMortality::NONE)
        {
            m_rainfall_mortality = 0;
        }
        else if(params()->vector_larval_rainfall_mortality == VectorRainfallMortality::SIGMOID)
        {
            if( rainfall * MM_PER_METER < params()->larval_rainfall_mortality_threshold * dt )
            {
                m_rainfall_mortality = 0;
            }
            else
            {
                m_rainfall_mortality = (rainfall * MM_PER_METER - params()->larval_rainfall_mortality_threshold * dt) / (params()->larval_rainfall_mortality_threshold * dt);
            }
        }
        else if(params()->vector_larval_rainfall_mortality == VectorRainfallMortality::SIGMOID_HABITAT_SHIFTING)
        {
            if( rainfall * MM_PER_METER < params()->larval_rainfall_mortality_threshold * dt * ( (m_max_larval_capacity - m_current_larval_capacity) / m_max_larval_capacity ) )
            {
                m_rainfall_mortality = 0;
            }
            else
            {
                m_rainfall_mortality = ( rainfall * MM_PER_METER - params()->larval_rainfall_mortality_threshold * dt * ( (m_max_larval_capacity - m_current_larval_capacity) / m_max_larval_capacity ) ) / (params()->larval_rainfall_mortality_threshold * dt);
            }
        }
        else
        {
            throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "params()->vector_larval_rainfall_mortality", params()->vector_larval_rainfall_mortality, VectorRainfallMortality::pairs::lookup_key( params()->vector_larval_rainfall_mortality ) );
        }

        LOG_DEBUG_F("Update rainfall-driven larval mortality to %f\n", m_rainfall_mortality);
    }

    void VectorHabitat::AdvanceTotalLarvaCounts()
    {
        LOG_DEBUG_F( "Advancing total larval counts for last two time steps: [ %d %d ]\n", m_total_larva_count[CURRENT_TIME_STEP], m_total_larva_count[PREVIOUS_TIME_STEP] );

        m_total_larva_count[PREVIOUS_TIME_STEP]          = m_total_larva_count[CURRENT_TIME_STEP];
        m_total_larva_count[CURRENT_TIME_STEP]           = 0;
    }

    VectorHabitatType::Enum VectorHabitat::GetVectorHabitatType() const
    {
        return m_habitat_type;
    }

    float VectorHabitat::GetMaximumLarvalCapacity() const
    {
        return m_max_larval_capacity;
    }

    float VectorHabitat::GetCurrentLarvalCapacity() const
    {
        // Current capacity (as driven by rainfall/evaporation/etc.)
        // and larval habitat reduction (e.g. larvicide) are evolved separately.
        // However, a query for the current value will return the product.
        return m_current_larval_capacity * m_larvicide_habitat_scaling;
    }

    int32_t VectorHabitat::GetTotalLarvaCount(TimeStepIndex index) const
    {
        return m_total_larva_count.at(index);
    }

    void VectorHabitat::AddLarva(int32_t larva, float progress)
    {
        // Summation of larval population depends on larval density model
        // GRADUAL_INSTAR_SPECIFIC is Notre Dame larval dynamics
        if(params()->larval_density_dependence == LarvalDensityDependence::GRADUAL_INSTAR_SPECIFIC || params()->larval_density_dependence == LarvalDensityDependence::LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY)
        {
            // scaled by progress and by a factor of 10.0 to avoid float to int issues
            m_total_larva_count[CURRENT_TIME_STEP] += progress * larva;
        }
        else
        {
            m_total_larva_count[CURRENT_TIME_STEP] += larva;
        }
        LOG_DEBUG_F("Adding %d larva in current timestep (progress = %f).  Total now equals %d.\n", larva, progress, m_total_larva_count[CURRENT_TIME_STEP]);
    }

    void VectorHabitat::AddEggs(int32_t eggs)
    {
        m_new_egg_count += eggs;
        LOG_DEBUG_F("Adding %d newly laid eggs.  Total now equals %d.\n", eggs,  m_new_egg_count);
    }

    void VectorHabitat::IncrementMaxLarvalCapacity(float capacity)
    {
        m_max_larval_capacity += capacity;
        LOG_DEBUG_F("Adding %f to maximum larval capacity.  Total now equals %f.\n", capacity,  m_max_larval_capacity);
    }

    float VectorHabitat::GetOvipositionTrapKilling() const
    {
        return m_oviposition_trap_killing;
    }

    float VectorHabitat::GetArtificialLarvalMortality() const
    {
        return m_artificial_larval_mortality;
    }

    float VectorHabitat::GetLarvicideHabitatScaling() const
    {
        return m_larvicide_habitat_scaling;
    }

    float VectorHabitat::GetLocalLarvalGrowthModifier() const
    {
        int32_t previous_larva_count = GetTotalLarvaCount(PREVIOUS_TIME_STEP);
        float larvalhabitat = GetCurrentLarvalCapacity();

        // Larval growth modifier defaults to 1
        float locallarvalgrowthmod = 1.0;

        // However, larval growth rate can be slowed by overpopulation, using total larva count from last step
        if ( previous_larva_count > larvalhabitat )
        {  
            locallarvalgrowthmod = larvalhabitat / previous_larva_count;
        }

        return locallarvalgrowthmod;
    }

    float VectorHabitat::GetLocalLarvalMortality(float species_aquatic_mortality, float progress) const
    {
        float larvalhabitat = GetCurrentLarvalCapacity();
        if (larvalhabitat <= 0 || progress <= 0) 
        {
            return 1.0f;  // No habitat = complete mortality
        }

        // Number of larva previous time step in species-specific habitat, 
        // basically how much food was eaten, does NOT count eggs laid which do not eat until this time step
        int   previous_larva_count          = m_total_larva_count[PREVIOUS_TIME_STEP];

        // Baseline aquatic mortality
        float locallarvalmortality = species_aquatic_mortality;

        switch ( params()->larval_density_dependence )
        {
            // Linear-&-uniform increase in mortality 
            case LarvalDensityDependence::UNIFORM_WHEN_OVERPOPULATION:
                if( previous_larva_count > larvalhabitat )
                {
                    locallarvalmortality *= ( previous_larva_count / larvalhabitat );
                }
                break;

            // Larval competition with Notre Dame larval dynamics
            case LarvalDensityDependence::GRADUAL_INSTAR_SPECIFIC:
                locallarvalmortality *= exp(previous_larva_count / ( (progress * params()->larvalDensityMortalityScalar + params()->larvalDensityMortalityOffset) * larvalhabitat) );
                break;

            case LarvalDensityDependence::LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY:
                locallarvalmortality *= exp(previous_larva_count / ( progress * params()->larvalDensityMortalityScalar * larvalhabitat) );
                break;

            case LarvalDensityDependence::NO_DENSITY_DEPENDENCE:
            case LarvalDensityDependence::DENSITY_DELAYED_GROWTH_NOT_MORTALITY:
                locallarvalmortality *= 1.0;
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "params()->larval_density_dependence", params()->larval_density_dependence, LarvalDensityDependence::pairs::lookup_key( params()->larval_density_dependence ) );
        }

        return locallarvalmortality;
    }

    float VectorHabitat::GetRainfallMortality() const
    {
        return m_rainfall_mortality;
    }

    float VectorHabitat::GetEggCrowdingCorrection() const
    {
        return m_egg_crowding_correction;
    }

    const SimulationConfig* VectorHabitat::params() const
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }
}