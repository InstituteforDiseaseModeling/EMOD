/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Infection.h"

namespace Kernel
{
    class InfectionPyConfig : public JsonConfigurable
    {
        friend class IndividualPy;
        GET_SCHEMA_STATIC_WRAPPER(InfectionPyConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        bool Configure( const Configuration* config );

    protected:
       
    };

    class IInfectionPy : public ISupports
    {
        public:
        virtual void Clear() = 0;
    };

    class InfectionPy
        : public Infection
        , public IInfectionPy 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static InfectionPy *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);
        virtual ~InfectionPy(void);

        virtual void SetParameters(StrainIdentity* infstrain = NULL, int incubation_period_override = -1);
        virtual void InitInfectionImmunology(Susceptibility* _immunity);
        virtual void Update(float dt, ISusceptibilityContext* _immunity = NULL) override;
        void SetMCWeightOfHost(float ind_mc_weight);
        virtual void Clear();

        // InfectionPyReportable methods
    protected:
        InfectionPy(); 

        const SimulationConfig* params();

        InfectionPy(IIndividualHumanContext *context);
        void Initialize(suids::suid _suid);

    private:
    };
}

