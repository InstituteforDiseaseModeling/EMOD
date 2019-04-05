/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "SusceptibilityEnvironmental.h"
#include "StrainIdentity.h"
#include "TyphoidDefs.h"

#ifdef ENABLE_TYPHOID

namespace Kernel
{
    class SusceptibilityTyphoidConfig: public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityTyphoidConfig)

        friend class IndividualHumanTyphoid;

    public:
        virtual bool Configure( const Configuration* config );
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    };

    class ISusceptibilityTyphoid : public ISupports
    {
    public:
        //virtual void SetNewInfectionByStrain(StrainIdentity* infection_strain) = 0;
        //immunity
   };

    class ISusceptibilityTyphoidReportable : public ISupports
    {
        public:
        //virtual void GetSheddingTiter(float sheddingTiters[])               const = 0; 
    };

    class SusceptibilityTyphoid :
        public SusceptibilityEnvironmental,
        public SusceptibilityTyphoidConfig,
        public ISusceptibilityTyphoid,
        public ISusceptibilityTyphoidReportable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

    protected:
    public:
        static SusceptibilityTyphoid *CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod);
        void Initialize(float _age, float _immmod, float _riskmod);
        SusceptibilityTyphoid(IIndividualHumanContext *context);
        SusceptibilityTyphoid() {}
        virtual ~SusceptibilityTyphoid(void);

        virtual void Update(float dt = 0.0);
        
    private:
    };
}
#endif
