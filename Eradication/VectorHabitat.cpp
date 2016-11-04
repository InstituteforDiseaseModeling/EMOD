/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "VectorHabitat.h"

#include "Climate.h"
#include "Contexts.h"
#include "Exceptions.h"
#include "Log.h"
#include "SimulationConfig.h" // TODO: long-term, it seems that lloffset should belong to Node and the larval decay rates and rainfall mortality thresholds to this class.
#include "Vector.h"
#include "INodeContext.h"
#include "NodeEventContext.h"   // for INodeEventContext
#include "VectorContexts.h"
#include "VectorParameters.h"

static const char * _module = "VectorHabitat";

namespace Kernel
{
    VectorHabitat::VectorHabitat( VectorHabitatType::Enum type )
        : m_habitat_type(type)
        , m_max_larval_capacity(0.0f)
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

    bool VectorHabitat::Configure( const Configuration* inputJson )
    {
        if( m_habitat_type != VectorHabitatType::LINEAR_SPLINE )
        {
            const char* type_str = VectorHabitatType::pairs::lookup_key( m_habitat_type );
            initConfigTypeMap( type_str, &m_max_larval_capacity, "TBD", 0.0f, FLT_MAX, 1E10 );

        }
        return JsonConfigurable::Configure( inputJson );
    }

    bool LinearSplineHabitat::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Max_Larval_Capacity", &m_max_larval_capacity, "TBD", 0.0f, FLT_MAX, 1E10 );
        initConfigComplexType( "Capacity_Distribution_Per_Year", &capacity_distribution, "TBD" );

        // -----------------------------------------------------------------------------------------------
        // --- This is a little different.  It is assumed that the inputJson element is the entire element
        // --- with the LINEAR_SPLINE as the name of the element.  Hence, we need to get the sub-elements
        // --- for JsonConfigurable.
        // -----------------------------------------------------------------------------------------------
        Configuration* sub_element = nullptr;
        if( inputJson != nullptr )
        {
            const char* type_str = VectorHabitatType::pairs::lookup_key( VectorHabitatType::LINEAR_SPLINE );
            sub_element = Configuration::CopyFromElement( (*inputJson)[type_str] );
        }
        bool ret = VectorHabitat::Configure( sub_element );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( capacity_distribution.size() < 1 )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Capacity_Distribution_Per_Year has zero values and must have at least one." );
            }
            if( capacity_distribution.begin()->first != 0.0 )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "The first entry in the Capacity_Distribution_Per_Year.Times array must be zero." );
            }
        }
        delete sub_element;
        sub_element = nullptr;
        return ret;
    }

    IVectorHabitat* VectorHabitat::CreateHabitat( VectorHabitatType::Enum type, const Configuration* inputJson )
    {
        IVectorHabitat* p_habitat = nullptr;
        switch ( type )
        {
            case VectorHabitatType::CONSTANT:
                p_habitat = new ConstantHabitat();
                break;
            case VectorHabitatType::TEMPORARY_RAINFALL:
                p_habitat = new TemporaryRainfallHabitat();
                break;
            case VectorHabitatType::WATER_VEGETATION:
                p_habitat = new WaterVegetationHabitat();
                break;
            case VectorHabitatType::HUMAN_POPULATION:
                p_habitat = new HumanPopulationHabitat();
                break;
            case VectorHabitatType::BRACKISH_SWAMP: 
                p_habitat = new BrackishSwampHabitat();
                break;
            case VectorHabitatType::LINEAR_SPLINE:
                p_habitat = new LinearSplineHabitat();
                break;
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "type", type, VectorHabitatType::pairs::lookup_key(type) );
        }
        p_habitat->Configure( inputJson );

        return p_habitat;
    }

    void VectorHabitat::Update( float dt, INodeContext* node, const std::string& species )
    {
        // (1) Calculate the egg-crowding correction based on eggs, larva, and habitat from previous step
        CalculateEggCrowdingCorrection();

        // (2) Update the current larval capacity based on local climate and maximum capacity
        UpdateCurrentLarvalCapacity(dt, node);

        // (3) Update the larval-habitat-specific probabilities related to node-targeted interventions
        UpdateLarvalProbabilities( dt, node, species );

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

        switch ( params()->vector_params->egg_saturation )
        {
            case EggSaturation::NO_SATURATION:
                m_egg_crowding_correction = 1.0;
                break;

            // Eggs divided up by habitat, neweggs holds number of female eggs.  Correcting for overcrowding, applies to all eggs evenly
            // Will hatch equal number of male larva, but these don't take up larval habitat in the calculations, to keep compatible with older results

            case EggSaturation::SATURATION_AT_OVIPOSITION:
                if (larvalhabitat > total_larva_count)
                {
                    if (m_new_egg_count > (larvalhabitat - total_larva_count))
                    { 
                        m_egg_crowding_correction = float(larvalhabitat - total_larva_count) / float(m_new_egg_count);
                    }
                    else
                    {
                        m_egg_crowding_correction = 1.0;
                    }
                }
                else
                {
                    // Habitat is full.  No new eggs.
                    m_egg_crowding_correction = 0;
                }
                break;

            case EggSaturation::SIGMOIDAL_SATURATION:
                if (larvalhabitat > 0)
                {
                    m_egg_crowding_correction = exp(-total_larva_count/larvalhabitat);
                }
                else
                {
                    m_egg_crowding_correction = 0.0;
                }
                break;

            default:
                throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__,
                                                        "params()->egg_saturation", params()->vector_params->egg_saturation,
                                                        EggSaturation::pairs::lookup_key(params()->vector_params->egg_saturation));
        }
        LOG_VALID_F( "m_egg_crowding_correction calculated as %f based on enum value %s, larval_habitat = %f, total_larval_count = %f, m_new_egg_count = %d\n",
                             m_egg_crowding_correction,
                             EggSaturation::pairs::lookup_key(params()->vector_params->egg_saturation),
                             larvalhabitat,
                             total_larva_count,
                             m_new_egg_count
                           );

        // After egg-crowding calculation has been calculated, reset new egg counter
        m_new_egg_count = 0;
    }

   void VectorHabitat::UpdateCurrentLarvalCapacity(float dt, INodeContext* node)
    {
        /* No-op */
    }

    void ConstantHabitat::UpdateCurrentLarvalCapacity(float dt, INodeContext* node)
    {
        LOG_DEBUG_F("Habitat type = CONSTANT, Max larval capacity = %f\n", m_max_larval_capacity);
        m_current_larval_capacity = m_max_larval_capacity * params()->lloffset * params()->lloffset;
    }

    void TemporaryRainfallHabitat::UpdateCurrentLarvalCapacity(float dt, INodeContext* node)
    {
        const Climate* localWeather = node->GetLocalWeather();
        LOG_DEBUG_F("Habitat type = TEMPORARY_RAINFALL, Max larval capacity = %f\n", m_max_larval_capacity);
        m_current_larval_capacity += float(localWeather->accumulated_rainfall() * m_max_larval_capacity * params()->lloffset * params()->lloffset - m_current_larval_capacity * exp(LARVAL_HABITAT_FACTOR1 / (localWeather->airtemperature() + CELSIUS_TO_KELVIN)) * LARVAL_HABITAT_FACTOR2 * params()->vector_params->tempHabitatDecayScalar * sqrt(LARVAL_HABITAT_FACTOR3 / (localWeather->airtemperature() + CELSIUS_TO_KELVIN)) * (1.0f - localWeather->humidity()) * dt);
    }

    void WaterVegetationHabitat::UpdateCurrentLarvalCapacity(float dt, INodeContext* node)
    {
        const Climate* localWeather = node->GetLocalWeather();
        LOG_DEBUG_F("Habitat type = WATER_VEGETATION, Max larval capacity = %f\n", m_max_larval_capacity);
        m_current_larval_capacity += localWeather->accumulated_rainfall() * m_max_larval_capacity * params()->lloffset * params()->lloffset - params()->vector_params->semipermanentHabitatDecayRate * dt * m_current_larval_capacity;
    }

    void HumanPopulationHabitat::UpdateCurrentLarvalCapacity(float dt, INodeContext* node)
    {
        LOG_DEBUG_F("Habitat type = HUMAN_POPULATION, Max larval capacity = %f\n", m_max_larval_capacity);
        m_current_larval_capacity = m_max_larval_capacity * node->GetStatPop();
    }

    void BrackishSwampHabitat::UpdateCurrentLarvalCapacity(float dt, INodeContext* node)
    {
        const Climate* localWeather = node->GetLocalWeather();
        LOG_DEBUG_F("Habitat type = BRACKISH_SWAMP, Max larval capacity = %f\n", m_max_larval_capacity);
        m_current_larval_capacity += localWeather->accumulated_rainfall() * float(MM_PER_METER) / params()->vector_params->mmRainfallToFillSwamp * m_max_larval_capacity * params()->lloffset * params()->lloffset - params()->vector_params->semipermanentHabitatDecayRate * dt * m_current_larval_capacity;
        if(m_current_larval_capacity > m_max_larval_capacity * params()->lloffset * params()->lloffset)
        {
            m_current_larval_capacity = m_max_larval_capacity * params()->lloffset * params()->lloffset;
        }
    }

    void LinearSplineHabitat::UpdateCurrentLarvalCapacity(float dt, INodeContext* node)
    {
        float scale = capacity_distribution.getValueLinearInterpolation( day_of_year );
        float area = params()->lloffset * params()->lloffset;
        float new_capacity = m_max_larval_capacity * scale * area;

        if( m_current_larval_capacity != new_capacity )
        {
            LOG_INFO_F("LinearSplineHabitat: Habitat_Changed - day_of_year=%f  area = %f  scale=%f  max_capacity=%f  current_capacity=%f  new_capacity=%f\n", 
                day_of_year, area, scale, m_max_larval_capacity, m_current_larval_capacity,new_capacity);
            m_current_larval_capacity = new_capacity;
        }

        day_of_year += dt;
        if( day_of_year >= DAYSPERYEAR )
        {
            day_of_year = 0.0;
        }
    }

    void VectorHabitat::UpdateLarvalProbabilities( float dt, INodeContext* node, const std::string& species )
    {
        // TODO: if this querying gets tedious, we can pass it in as an argument from NodeVector?
        INodeVectorInterventionEffects* invie = nullptr;
        if (s_OK != node->GetEventContext()->QueryInterface(GET_IID(INodeVectorInterventionEffects), (void**)&invie))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "GetEventContext()", "INodeVectorInterventionEffects", "INodeEventContext" );
        }

        m_oviposition_trap_killing    = invie->GetOviTrapKilling(m_habitat_type);
        m_artificial_larval_mortality = EXPCDF( -dt * ( invie->GetLarvalKilling(m_habitat_type) ) );
        m_larvicide_habitat_scaling   = 1.0f - invie->GetLarvalHabitatReduction( m_habitat_type, species );

        LOG_DEBUG_F("Updated larval probabilites: oviposition-trap killing = %f, artificial larval mortality = %f, larvicide habitat reduction = %f\n", m_oviposition_trap_killing, m_artificial_larval_mortality, m_larvicide_habitat_scaling);
    }

    void VectorHabitat::UpdateRainfallMortality(float dt, float rainfall)
    {
        // TODO: this calculation is only habitat dependent for the case of VectorRainfallMortality::SIGMOID_HABITAT_SHIFTING
        //       that was designed to make "flushing" of BRACKISH_SWAMP habitat more likely if the habitat were almost full.
        //       in practice, this makes little difference for the Solomon Island rainfall patterns.

        // Paaijmans, K. P., M. O. Wandago, et al. (2007). "Unexpected High Losses of Anopheles gambiae Larvae Due to Rainfall." PLoS One 2(11): e1146.
        if ( params()->vector_params->vector_larval_rainfall_mortality == VectorRainfallMortality::NONE ||
             m_habitat_type != VectorHabitatType::BRACKISH_SWAMP )
        {
            m_rainfall_mortality = 0;
        }
        else if(params()->vector_params->vector_larval_rainfall_mortality == VectorRainfallMortality::SIGMOID)
        {
            if( rainfall * MM_PER_METER < params()->vector_params->larval_rainfall_mortality_threshold * dt )
            {
                m_rainfall_mortality = 0;
            }
            else
            {
                m_rainfall_mortality = (rainfall * MM_PER_METER - params()->vector_params->larval_rainfall_mortality_threshold * dt) / (params()->vector_params->larval_rainfall_mortality_threshold * dt);
            }
        }
        else if(params()->vector_params->vector_larval_rainfall_mortality == VectorRainfallMortality::SIGMOID_HABITAT_SHIFTING)
        {
            float full_habitat = m_max_larval_capacity * params()->lloffset * params()->lloffset;
            float fraction_empty = (full_habitat - m_current_larval_capacity) / full_habitat;
            if( rainfall * MM_PER_METER < params()->vector_params->larval_rainfall_mortality_threshold * dt * fraction_empty )
            {
                m_rainfall_mortality = 0;
            }
            else
            {
                m_rainfall_mortality = (rainfall * MM_PER_METER - params()->vector_params->larval_rainfall_mortality_threshold * dt * fraction_empty) / (params()->vector_params->larval_rainfall_mortality_threshold * dt);
            }
        }
        else
        {
            throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "params()->vector_larval_rainfall_mortality", params()->vector_params->vector_larval_rainfall_mortality, VectorRainfallMortality::pairs::lookup_key( params()->vector_params->vector_larval_rainfall_mortality ) );
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
        if( params()->vector_params->larval_density_dependence == LarvalDensityDependence::GRADUAL_INSTAR_SPECIFIC || 
            params()->vector_params->larval_density_dependence == LarvalDensityDependence::LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY )
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

    void VectorHabitat::SetMaximumLarvalCapacity(float capacity)
    {
        m_max_larval_capacity = capacity;
        LOG_DEBUG_F("Setting %f to maximum larval capacity.  Total now equals %f.\n", capacity,  m_max_larval_capacity);
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

        LOG_VALID_F( "%s returning %f based on current larval count (%u), capacity (%f).\n", __FUNCTION__, locallarvalgrowthmod, previous_larva_count, larvalhabitat );
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

        switch ( params()->vector_params->larval_density_dependence )
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
            case LarvalDensityDependence::LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY:
                locallarvalmortality *= exp(previous_larva_count / ( (progress * params()->vector_params->larvalDensityMortalityScalar + params()->vector_params->larvalDensityMortalityOffset) * larvalhabitat) );
                break;

            case LarvalDensityDependence::NO_DENSITY_DEPENDENCE:
            case LarvalDensityDependence::DENSITY_DELAYED_GROWTH_NOT_MORTALITY:
                locallarvalmortality *= 1.0;
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "params()->larval_density_dependence", params()->vector_params->larval_density_dependence, LarvalDensityDependence::pairs::lookup_key( params()->vector_params->larval_density_dependence ) );
        }

        LOG_VALID_F( "%s returning %f based on previous larval count (%d), current larval count (%d), and capacity (%f).\n",
                     __FUNCTION__,
                     locallarvalmortality,
                     previous_larva_count,
                     m_total_larva_count[CURRENT_TIME_STEP],
                     larvalhabitat
                    );
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


    void VectorHabitat::serialize(IArchive& ar, VectorHabitat* obj)
    {
        VectorHabitat& habitat = *obj;
        ar.labelElement("m_habitat_type") & (uint32_t&)habitat.m_habitat_type;
        ar.labelElement("m_max_larval_capacity") & habitat.m_max_larval_capacity;
        ar.labelElement("m_current_larval_capacity") & habitat.m_current_larval_capacity;
        ar.labelElement("m_total_larva_count") & habitat.m_total_larva_count;
        ar.labelElement("m_new_egg_count") & habitat.m_new_egg_count;
        ar.labelElement("m_oviposition_trap_killing") & habitat.m_oviposition_trap_killing;
        ar.labelElement("m_artificial_larval_mortality") & habitat.m_artificial_larval_mortality;
        ar.labelElement("m_larvicide_habitat_scaling") & habitat.m_larvicide_habitat_scaling;
        ar.labelElement("m_rainfall_mortality") & habitat.m_rainfall_mortality;
        ar.labelElement("m_egg_crowding_correction") & habitat.m_egg_crowding_correction;
    }

    void serialize(IArchive& ar, map<VectorHabitatType::Enum, float>& mapping)
    {
        size_t count=  ar.IsWriter() ? mapping.size() : -1;

        ar.startArray(count);
        if (ar.IsWriter())
        {
            for (auto entry : mapping)
            {
                ar.startObject();
                    ar.labelElement("key") & (uint32_t&)entry.first;
                    ar.labelElement("value") & entry.second;
                ar.endObject();
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                VectorHabitatType::Enum type;
                float value;
                ar.startObject();
                    ar.labelElement("key") & (uint32_t&)type;
                    ar.labelElement("value") & value;
                ar.endObject();
                mapping[type] = value;
            }
        }
        ar.endArray();
    }

    ConstantHabitat::ConstantHabitat()
        : VectorHabitat(VectorHabitatType::CONSTANT)
    {
    }

    TemporaryRainfallHabitat::TemporaryRainfallHabitat()
        : VectorHabitat(VectorHabitatType::TEMPORARY_RAINFALL)
    {
        /* TODO: configuration of habitat decay parameters etc. */
    }

    WaterVegetationHabitat::WaterVegetationHabitat()
        : VectorHabitat(VectorHabitatType::WATER_VEGETATION)
    {
        /* TODO: configuration of habitat decay parameters etc. */
    }

    HumanPopulationHabitat::HumanPopulationHabitat()
        : VectorHabitat(VectorHabitatType::HUMAN_POPULATION)
    {
    }

    BrackishSwampHabitat::BrackishSwampHabitat()
        : VectorHabitat(VectorHabitatType::BRACKISH_SWAMP)
    {
        /* TODO: configuration of habitat decay parameters, threshold, rainfall mortality, etc. */
    }

    LinearSplineHabitat::LinearSplineHabitat()
        : VectorHabitat(VectorHabitatType::LINEAR_SPLINE)
        , day_of_year(0)
        , capacity_distribution(0.0f,365.0f,0.0f,FLT_MAX)
    {
    }

    REGISTER_SERIALIZABLE(ConstantHabitat);

    void ConstantHabitat::serialize(IArchive& ar, ConstantHabitat* obj)
    {
        VectorHabitat::serialize(ar, obj);
        // no-op
    }

    REGISTER_SERIALIZABLE(TemporaryRainfallHabitat);

    void TemporaryRainfallHabitat::serialize(IArchive& ar, TemporaryRainfallHabitat* obj)
    {
        VectorHabitat::serialize(ar, obj);
        // no-op
    }

    REGISTER_SERIALIZABLE(WaterVegetationHabitat);

    void WaterVegetationHabitat::serialize(IArchive& ar, WaterVegetationHabitat* obj)
    {
        VectorHabitat::serialize(ar, obj);
        // no-op
    }

    REGISTER_SERIALIZABLE(HumanPopulationHabitat);

    void HumanPopulationHabitat::serialize(IArchive& ar, HumanPopulationHabitat* obj)
    {
        VectorHabitat::serialize(ar, obj);
        // no-op
    }

    REGISTER_SERIALIZABLE(BrackishSwampHabitat);

    void BrackishSwampHabitat::serialize(IArchive& ar, BrackishSwampHabitat* obj)
    {
        VectorHabitat::serialize(ar, obj);
        // no-op
    }

    REGISTER_SERIALIZABLE(LinearSplineHabitat);

    void LinearSplineHabitat::serialize(IArchive& ar, LinearSplineHabitat* obj)
    {
        VectorHabitat::serialize(ar, obj);
        LinearSplineHabitat& habitat = *obj;
        ar.labelElement("day_of_year") & habitat.day_of_year;
        ar.labelElement("capacity_distribution") & habitat.capacity_distribution;
    }
}
