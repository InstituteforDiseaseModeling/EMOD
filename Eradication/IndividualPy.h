/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "InfectionPy.h"
#include "Individual.h"
#include "PythonSupport.h"

namespace Kernel
{
    class SusceptibilityPy;

    class IIndividualHumanPy : public ISupports
    {
    };

    class IndividualHumanPy : public IndividualHuman, public IIndividualHumanPy
    {
        friend class SimulationPy;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        static IndividualHumanPy *CreateHuman(INodeContext *context, suids::suid id, float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0);
        virtual ~IndividualHumanPy(void);

        virtual void CreateSusceptibility(float imm_mod = 1.0, float risk_mod = 1.0);
        virtual void ExposeToInfectivity(float dt, TransmissionGroupMembership_t transmissionGroupMembership);

    protected:

        // New Exposure Pattern
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route );

        IndividualHumanPy(suids::suid id = suids::nil_suid(), float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0);
        virtual void setupInterventionsContainer();
        virtual void PropagateContextToDependents();

        virtual void UpdateGroupMembership() override;
        virtual void UpdateInfectiousness(float dt);
        virtual void Update(float currenttime, float dt);
        virtual Infection* createInfection(suids::suid _suid);
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain = nullptr, int incubation_period_override = -1);
        virtual HumanStateChange GetStateChange() const;

        std::string processPrePatent( float dt );

        // pydemo infection state
        std::string state_to_report; // pydemo status of individual
        std::string last_state_reported; // previous pydemo status of individual
        int _infection_count;     // number of times infected;
        TransmissionRoute::Enum _routeOfInfection; // how did this person get infected?
        bool isDead;  // is this individual dead?
        bool state_changed;

        std::map<std::string, TransmissionGroupMembership_t> transmissionGroupMembershipByRoute;

    private:
#ifdef ENABLE_PYTHON
        PyObject * expose_vars;
#endif
        SusceptibilityPy * pydemo_susceptibility;
        std::map< TransmissionRoute::Enum, float > contagion_population_by_route;

        static void InitializeStaticsPy( const Configuration* config );
    };
}
