/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "IndividualAirborne.h"
#include "stdafx.h"

// These includes are only used for serialization
#include "InfectionTB.h"
#include "SusceptibilityTB.h"
#include "TBInterventionsContainer.h"
#include "TBContexts.h"

namespace Kernel
{
    class IInfectionIncidenceObserver;

    class IndividualHumanTB : public IndividualHumanAirborne, public IIndividualHumanTB2
    {
        friend class SimulationTB;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE()

    public:
        virtual ~IndividualHumanTB(void) { }
        static   IndividualHumanTB *CreateHuman(INodeContext *context, suids::suid _suid, float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0, float initial_poverty = 0.5f);
        virtual void InitializeHuman();
        virtual bool Configure( const Configuration* config );

        // Infections and Susceptibility
        virtual void CreateSusceptibility(float=1.0, float=1.0);
        virtual void UpdateInfectiousness(float dt);

        // These functions in IIndividualHumanTB
        virtual bool HasActiveInfection() const;
        virtual bool HasLatentInfection() const;
        virtual bool HasPendingRelapseInfection() const;
        virtual bool IsImmune() const;
        virtual bool IsMDR() const; 
        virtual bool IsSmearPositive() const;
        virtual bool IsOnTreatment() const; 
        virtual bool IsEvolvedMDR() const;
        virtual bool IsTreatmentNaive() const;
        virtual bool HasFailedTreatment() const;
        virtual bool HasEverRelapsedAfterTreatment() const;
        virtual bool IsFastProgressor() const;
        virtual bool IsExtrapulmonary() const;
        virtual bool HasActivePresymptomaticInfection() const;
        virtual float GetDurationSinceInitInfection() const;

        virtual int GetTime() const;

        //event observers for reporting
        virtual void RegisterInfectionIncidenceObserver( IInfectionIncidenceObserver*);
        virtual void UnRegisterAllObservers ( IInfectionIncidenceObserver *);

    protected:
        IndividualHumanTB(suids::suid id = suids::nil_suid(), float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0, float initial_poverty = 0.5f);

        // Factory methods
        virtual Infection* createInfection(suids::suid _suid);
        virtual void setupInterventionsContainer();
        virtual bool SetNewInfectionState(InfectionStateChange::_enum inf_state_change);

        //event observers for reporting
        std::set < IInfectionIncidenceObserver * > infectionIncidenceObservers;
        virtual void onInfectionIncidence();
        virtual void onInfectionMDRIncidence();
    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        // Serialization
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, IndividualHumanTB& human, const unsigned int  file_version );
#endif
    };
}
