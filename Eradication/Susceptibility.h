/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BoostLibWrapper.h"

#include "Contexts.h"
#include "Sugar.h"
#include "Configure.h"

class Configuration;

namespace Kernel
{
    class SusceptibilityConfig : public JsonConfigurable 
    {
        friend class Individual;

    public:
        virtual bool Configure( const Configuration* config ) override;

    protected:
        friend class Susceptibility;

        static bool  immune_decay;

        static float acqdecayrate;
        static float trandecayrate;
        static float mortdecayrate;
        static float baseacqupdate;
        static float basetranupdate;
        static float basemortupdate;
        static float baseacqoffset;
        static float basetranoffset;
        static float basemortoffset;

        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    };

    class Susceptibility : public ISusceptibilityContext
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static Susceptibility *Susceptibility::CreateSusceptibility(IIndividualHumanContext *context, float _age, float immmod, float riskmod);
        virtual ~Susceptibility();
        virtual void SetContextTo(IIndividualHumanContext* context);
        IIndividualHumanContext* GetParent();

        virtual void Update(float dt=0.0);
        virtual void UpdateInfectionCleared();

        // ISusceptibilityContext interfaces
        virtual float getAge() const override;
        virtual float getModAcquire() const override;
        virtual float GetModTransmit() const override;
        virtual float getModMortality() const override;
        virtual float getSusceptibilityCorrection() const;
        virtual bool  IsImmune() const;
        virtual void  InitNewInfection() override;

    protected:
        // current status
        float age;

        // immune modifiers
        float mod_acquire;
        float mod_transmit;
        float mod_mortality;

        float acqdecayoffset;
        float trandecayoffset;
        float mortdecayoffset;

        Susceptibility();
        Susceptibility(IIndividualHumanContext *context);
        /* clorton virtual */ void Initialize(float _age, float immmod, float riskmod) /* clorton override */;

        IIndividualHumanContext *parent;

        /* clorton virtual */ const SimulationConfig* params() /* clorton override */;

        DECLARE_SERIALIZABLE(Susceptibility);
    };
}
