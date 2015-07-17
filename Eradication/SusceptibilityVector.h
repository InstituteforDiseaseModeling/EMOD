/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "VectorContexts.h"
#include "Common.h"
#include "Susceptibility.h"

namespace Kernel
{
    class SimulationConfig;

    class SusceptibilityVectorConfig : public JsonConfigurable 
    {
        friend class IndividualHumanVector;
    public:
        virtual bool Configure( const Configuration* config );

    protected:
        // configurable mode of biting-risk age-dependence
        static AgeDependentBitingRisk::Enum age_dependent_biting_risk_type;
        static float m_newborn_biting_risk;

        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityVectorConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    };

    class SusceptibilityVector : public Susceptibility, public IVectorSusceptibilityContext, protected SusceptibilityVectorConfig
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static SusceptibilityVector *CreateSusceptibility(IIndividualHumanContext *context, float _age = (20*DAYSPERYEAR), float immmod = 1.0f, float riskmod = 1.0f);
        virtual ~SusceptibilityVector();

        virtual void Update(float dt=0.0);

        // IVectorSusceptibilityContext interface
        virtual float GetRelativeBitingRate(void) const;

        static float LinearBitingFunction(float);
        static float SurfaceAreaBitingFunction(float);

    protected:
        SusceptibilityVector();
        SusceptibilityVector(IIndividualHumanContext *context);
        void Initialize(float _age, float immmod, float riskmod);
        const SimulationConfig *params();
        float BitingRiskAgeFactor(float _age);

        // effect of heterogeneous biting explored in Smith, D. L., F. E. McKenzie, et al. (2007). "Revisiting the basic reproductive number for malaria and its implications for malaria control." PLoS Biol 5(3): e42.
        // also in Smith, D. L., J. Dushoff, et al. (2005). "The entomological inoculation rate and Plasmodium falciparum infection in African children." Nature 438(7067): 492-495.
        float m_relative_biting_rate;
        float m_age_dependent_biting_risk;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, SusceptibilityVector& sus, const unsigned int file_version );
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
     // IJsonSerializable Interfaces
     virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
     virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

    };


}
