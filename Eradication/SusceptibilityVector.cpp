/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include <math.h>

#include "Common.h"
#include "SusceptibilityVector.h"
#include "SimulationConfig.h"

#ifdef randgen
#undef randgen
#endif
#include "RANDOM.h"
#define randgen (parent->GetRng())

static const char * _module = "SusceptibilityVector";

namespace Kernel
{
    AgeDependentBitingRisk::Enum SusceptibilityVectorConfig::age_dependent_biting_risk_type = AgeDependentBitingRisk::OFF;
    float  SusceptibilityVectorConfig::m_newborn_biting_risk = 0.2f;
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Vector.Susceptibility,SusceptibilityVectorConfig)
    BEGIN_QUERY_INTERFACE_BODY(SusceptibilityVectorConfig)
    END_QUERY_INTERFACE_BODY(SusceptibilityVectorConfig)

    bool SusceptibilityVectorConfig::Configure( const Configuration* config )
    {
        initConfig( "Age_Dependent_Biting_Risk_Type", age_dependent_biting_risk_type, config, MetadataDescriptor::Enum("age_dependent_biting_risk_type", Age_Dependent_Biting_Risk_Type_DESC_TEXT, MDD_ENUM_ARGS(AgeDependentBitingRisk)) );
        if (age_dependent_biting_risk_type == AgeDependentBitingRisk::LINEAR || JsonConfigurable::_dryrun)
        {
            initConfigTypeMap( "Newborn_Biting_Risk_Multiplier", &m_newborn_biting_risk, Newborn_Biting_Risk_DESC_TEXT, 0.0f, 1.0f, 0.2f);
        }
        return JsonConfigurable::Configure( config );
    }

    SusceptibilityVector::SusceptibilityVector() : Kernel::Susceptibility()
    {
    }

    SusceptibilityVector::SusceptibilityVector(IIndividualHumanContext *context) : Kernel::Susceptibility(context)
    {
    }

    // QI stuff in case we want to use it more extensively
    BEGIN_QUERY_INTERFACE_DERIVED(SusceptibilityVector, Susceptibility)
        HANDLE_INTERFACE(IVectorSusceptibilityContext)
    END_QUERY_INTERFACE_DERIVED(SusceptibilityVector, Susceptibility)

    void SusceptibilityVector::Initialize(float _age, float immmod, float riskmod)
    {
        Susceptibility::Initialize(_age, immmod, riskmod);
        m_relative_biting_rate = riskmod;
        m_age_dependent_biting_risk = BitingRiskAgeFactor(_age);
    }

    SusceptibilityVector *SusceptibilityVector::CreateSusceptibility(IIndividualHumanContext *context, float _age, float immmod, float riskmod)
    {
        SusceptibilityVector *newsusceptibility = _new_ SusceptibilityVector(context);
        newsusceptibility->Initialize(_age, immmod, riskmod);

        return newsusceptibility;
    }

    SusceptibilityVector::~SusceptibilityVector()
    {
    }

    void SusceptibilityVector::Update(float dt)
    {
        // base-class behavior
        Susceptibility::Update(dt);

        // update age-based biting risk
        m_age_dependent_biting_risk = BitingRiskAgeFactor(age);
    }

    const SimulationConfig* SusceptibilityVector::params()
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    float SusceptibilityVector::GetRelativeBitingRate(void) const
    {
        return m_relative_biting_rate * m_age_dependent_biting_risk;
    }

    float SusceptibilityVector::BitingRiskAgeFactor(float _age)
    {
        float risk = 1.0f;
        switch(age_dependent_biting_risk_type)
        {
            case AgeDependentBitingRisk::OFF:
                // risk is independent by age
                break;

            case AgeDependentBitingRisk::LINEAR:
                risk = LinearBitingFunction(_age);
                break;

            case AgeDependentBitingRisk::SURFACE_AREA_DEPENDENT:
                risk = SurfaceAreaBitingFunction(_age);
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, 
                    "age_dependent_biting_risk_type", age_dependent_biting_risk_type, 
                    AgeDependentBitingRisk::pairs::lookup_key(age_dependent_biting_risk_type) );
        }

        LOG_DEBUG_F("Age-dependent biting-risk = %f for %0.2f-year-old individual.\n", risk, _age/DAYSPERYEAR);
        return risk;
    }

        float SusceptibilityVector::LinearBitingFunction(float _age)
        {
            if ( _age < 20 * DAYSPERYEAR )
            {
                // linear from birth to age 20 years
                float newborn_risk = m_newborn_biting_risk;
                return newborn_risk + _age * (1 - newborn_risk) / (20 * DAYSPERYEAR);
            }
            else 
            {
                return 1.0f;
            }
        }

        float SusceptibilityVector::SurfaceAreaBitingFunction(float _age)
        {
            // piecewise linear rising from birth to age 2
            // and then shallower slope to age 20
            float newborn_risk = 0.07f;
            float two_year_old_risk = 0.23f;
            if ( _age < 2 * DAYSPERYEAR )
            {
                return newborn_risk + _age * (two_year_old_risk - newborn_risk) / (2 * DAYSPERYEAR);
            }
            else if ( _age < 20 * DAYSPERYEAR )
            {
                return two_year_old_risk + (_age - 2 * DAYSPERYEAR) * (1 - two_year_old_risk) / ( (20 - 2) * DAYSPERYEAR );
            }
            else 
            {
                return 1.0f;
            }
        }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::SusceptibilityVector)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, SusceptibilityVector& sus, const unsigned int file_version )
    {
        ar & sus.m_relative_biting_rate;
        ar & sus.m_age_dependent_biting_risk;
        ar & boost::serialization::base_object<Susceptibility>(sus);
    }
    template void serialize( boost::archive::binary_iarchive&, SusceptibilityVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_iarchive&, SusceptibilityVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_skeleton_oarchive&, SusceptibilityVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_skeleton_iarchive&, SusceptibilityVector &obj, unsigned int file_version );
    template void serialize( boost::archive::binary_oarchive&, SusceptibilityVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_oarchive&, SusceptibilityVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::detail::content_oarchive&, SusceptibilityVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::detail::mpi_datatype_oarchive&, SusceptibilityVector &obj, unsigned int file_version );
}
#endif


#if USE_JSON_SERIALIZATION || USE_JSON_MPI

namespace Kernel {

    // IJsonSerializable Interfaces
    void SusceptibilityVector::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();

        root->Insert("m_relative_biting_rate", m_relative_biting_rate);
        root->Insert("m_age_dependent_biting_risk", m_age_dependent_biting_risk);
        Susceptibility::JSerialize(root, helper);;

        root->EndObject();
    }

    void SusceptibilityVector::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
    }
}

#endif

