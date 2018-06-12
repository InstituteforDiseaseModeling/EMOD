/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Susceptibility.h"

#ifdef ENABLE_PYTHON

namespace Kernel
{
    class StrainIdentity;

    class SusceptibilityPyConfig: public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityPyConfig)

        friend class IndividualHumanPy;

    public:
        virtual bool Configure( const Configuration* config );
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
        static float x_population_immunity;
    };

    class ISusceptibilityPy : public ISupports
    {
    public:
        //virtual void SetNewInfectionByStrain(StrainIdentity* infection_strain) = 0;
        //immunity
   };

    class ISusceptibilityPyReportable : public ISupports
    {
        public:
        //virtual void GetSheddingTiter(float sheddingTiters[])               const = 0; 
    };

    class SusceptibilityPy :
        public Susceptibility,
        public SusceptibilityPyConfig,
        public ISusceptibilityPy,
        public ISusceptibilityPyReportable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

    protected:
    public:
        static SusceptibilityPy *CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod);
        void Initialize(float _age, float _immmod, float _riskmod);
        SusceptibilityPy(IIndividualHumanContext *context);
    SusceptibilityPy() {}
        virtual ~SusceptibilityPy(void);

        virtual void Update(float dt = 0.0);

    private:
    };
}
#endif
