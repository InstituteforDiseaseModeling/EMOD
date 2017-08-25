/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "IndividualAirborne.h"
#include "stdafx.h"

// These includes are only used for serialization
#include "InfectionTB.h"
#include "TBContexts.h"

namespace Kernel
{
    class IndividualHumanTBConfig : public IndividualHumanAirborneConfig
    {
    protected:
        friend class IndividualHumanTB;
    };

    class IInfectionIncidenceObserver;

    class IndividualHumanTB : public IndividualHumanAirborne, public IIndividualHumanTB2
    {
        friend class SimulationTB;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE()

    public:
        virtual ~IndividualHumanTB(void) { }
        static   IndividualHumanTB *CreateHuman(INodeContext *context, suids::suid _suid, float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0, float initial_poverty = 0.5f);
        virtual void InitializeHuman() override;

        // Infections and Susceptibility
        virtual void CreateSusceptibility(float=1.0, float=1.0) override;
        virtual void UpdateInfectiousness(float dt) override;

        // These functions in IIndividualHumanTB
        virtual bool HasActiveInfection() const override;
        virtual bool HasLatentInfection() const override;
        virtual bool HasPendingRelapseInfection() const override;
        virtual bool IsImmune() const override;
        virtual bool IsMDR() const override;
        virtual bool IsSmearPositive() const override;
        virtual bool IsOnTreatment() const override;
        virtual bool IsEvolvedMDR() const override;
        virtual bool IsTreatmentNaive() const override;
        virtual bool HasFailedTreatment() const override;
        virtual bool HasEverRelapsedAfterTreatment() const override;
        virtual bool IsFastProgressor() const override;
        virtual bool IsExtrapulmonary() const override;
        virtual bool HasActivePresymptomaticInfection() const override;
        virtual float GetDurationSinceInitInfection() const override;

        virtual int GetTime() const override;

        //event observers for reporting
        virtual void RegisterInfectionIncidenceObserver( IInfectionIncidenceObserver*);
        virtual void UnRegisterAllObservers ( IInfectionIncidenceObserver *);

    protected:
        IndividualHumanTB(suids::suid id = suids::nil_suid(), float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0, float initial_poverty = 0.5f);

        // Factory methods
        virtual IInfection* createInfection(suids::suid _suid) override;
        virtual void setupInterventionsContainer() override;
        virtual bool SetNewInfectionState(InfectionStateChange::_enum inf_state_change) override;

        //event observers for reporting
        std::vector < IInfectionIncidenceObserver * > infectionIncidenceObservers;
        virtual void onInfectionIncidence() override;
        virtual void onInfectionMDRIncidence() override;

        static void IndividualHumanTB::InitializeStaticsTB( const Configuration* config );

        DECLARE_SERIALIZABLE(IndividualHumanTB);
    };
}
