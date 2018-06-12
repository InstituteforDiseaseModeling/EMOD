/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "IndividualAirborne.h"
#include <list>
#include <vector>
#include "Exceptions.h"

// These includes are only used for serialization
#include "InfectionTB.h"
#include "SusceptibilityTB.h"
#include "TBInterventionsContainer.h"

//These includes are only used for serialization
#include "InfectionHIV.h"
#include "SusceptibilityHIV.h"
#include "HIVInterventionsContainer.h"
#include "IIndividualHumanHIV.h"
#include "TBContexts.h"

namespace Kernel
{
    class IInfectionTB;
    // This class has no need yet for flags beyond those in the base class
    class IIndividualHumanCoInfection : public ISupports
    {
        friend struct IInfection;

    public:
        virtual void ModActivate()  = 0;
        virtual void SetForwardCD4(std::vector<float> & )  = 0;
        virtual void SetForwardTBAct( std::vector<float> & ) = 0;
        virtual void InitiateART() = 0; 
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain = nullptr, int incubation_period_override = -1) = 0;
        virtual void SetTBActivationVector( const std::vector<float>& ) = 0;
        virtual void LifeCourseLatencyUpdateAll() = 0;

        virtual bool HasActiveInfection() const = 0;
        virtual bool HasLatentInfection() const = 0;
        virtual bool IsImmune() const = 0;
        virtual bool HasHIV() const = 0;
        virtual bool HasTB() const = 0;
        virtual bool IsMDR() const = 0; 
        virtual bool IsSmearPositive() const = 0;
        virtual bool HasActivePresymptomaticInfection() const = 0;
        virtual bool IsExtrapulmonary() const = 0;
        virtual bool HasPendingRelapseInfection() const = 0;
        virtual bool GetExogenousTBStateChange() const = 0;

        virtual NaturalNumber              GetViralLoad() const = 0;
        virtual float                      GetCD4() const = 0;
        virtual float                      GetNextLatentActivation(float time) const = 0;
        virtual float                      GetTBCD4InfectiousnessMap( float ) const = 0;
        virtual float                      GetCD4SusceptibilityMap( float ) const = 0;
        virtual float                      GetCD4PrimaryMap(float) const = 0;
        virtual NewInfectionState::_enum   GetNewInfectionState() const = 0;
        virtual ITBInterventionsContainer* GetTBInterventionsContainer() const = 0;
        
        virtual const std::vector<float>& GetTBActivationVector() const = 0;
        virtual vector <float>            GetForwardCD4Act() = 0;
        virtual map <float,float>         &GetCD4Map() const = 0;
        virtual const std::list< Susceptibility* > &GetSusceptibilityList() = 0;
        virtual float GetImmunityReducedAcquire() = 0;
    };

    //class IndividualHumanCoInfection : public IIndividualHumanCoInfection, public IIndividualHumanTB, public IndividualHumanAirborne, public IIndividualHumanHIV 
    class IndividualHumanCoInfectionConfig : public IndividualHumanConfig
    {
        GET_SCHEMA_STATIC_WRAPPER( IndividualHumanCoInfectionConfig )
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        virtual bool Configure( const Configuration* config ) override;
        static bool enable_coinfection;

    protected:
        friend class IndividualHumanCoInfection;
        friend class NodeTBHIV;

        static map <float,float> CD4_act_map;
        static float ART_extra_reactivation_reduction;
        static map <float, float> TB_CD4_Infectiousness_Map;
        static map <float, float> TB_CD4_Susceptibility_Map;
        static map<float, float> TB_CD4_Primary_Progression_Map;
        static vector <float> TB_CD4_infectiousness;
        static vector <float> TB_CD4_susceptibility;
        static vector <float> TB_CD4_strata_for_infectiousness_susceptibility;
        static vector <float> TB_CD4_Primary;
        static bool enable_coinfection_mortality;
        static float coinfection_mortality_on_ART;
        static float coinfection_mortality_off_ART;

        static bool enable_exogenous; 

        virtual void ConstructTBInfSusCD4Maps();
    };

    class IndividualHumanCoInfection : public IIndividualHumanCoInfection, public IIndividualHumanHIV, public IIndividualHumanTB, public IndividualHumanAirborne 
    {
        friend class SimulationTBHIV;
        friend class Node;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE()

    public:
        virtual ~IndividualHumanCoInfection(void);
        static   IndividualHumanCoInfection *CreateHuman(INodeContext *context, suids::suid _suid, float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0);
        IndividualHumanCoInfection();  // just trying to make compiler happy: TBD

        // Infections and Susceptibility
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain = nullptr, int incubation_period_override = -1) override;
        virtual void AcquireNewInfectionHIV( const IStrainIdentity *infstrain = nullptr, int incubation_period_override = -1); // does not override

        virtual void CreateSusceptibility(float=1.0, float=1.0) override;
        virtual void UpdateInfectiousness(float dt) override;
        virtual void Update(float currenttime, float dt) override;
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route = TransmissionRoute::TRANSMISSIONROUTE_CONTACT ) override;
        virtual void CheckHIVVitalDynamics(float=1.0); // non-disease mortality, does not override
         //Deaths
        virtual void Die(HumanStateChange) override;
        virtual bool IsDead() const override;

        // For reporting of individual's infection status to NodeTB
        // TODO: Consider giving the individual an infection count to avoid repeated function calls

        virtual bool HasActiveInfection() const override;
        virtual bool HasLatentInfection() const override;
        virtual bool HasTB() const override;
        virtual bool IsImmune() const override;
        virtual inline NewInfectionState::_enum GetNewInfectionState() const override { return m_new_infection_state; }
        virtual bool IsMDR() const override;
        virtual int GetTime() const override;
        virtual bool IsSmearPositive() const override;
        virtual float GetCD4() const override;
        virtual void SetForwardCD4(vector <float> &vin) override  {CD4_forward_vector = vin; };
        virtual void SetForwardTBAct( std::vector<float> & vin) override;
        virtual vector <float> GetForwardCD4Act() override {return CD4_forward_vector;};

        virtual map <float,float> &GetCD4Map() const {return IndividualHumanCoInfectionConfig::CD4_act_map;} 
        virtual void LifeCourseLatencyTimerUpdate( IInfectionTB*); // does not override

        virtual bool HasActivePresymptomaticInfection() const;
        virtual bool IsExtrapulmonary() const override;
        virtual void InitiateART() override;
        virtual void LifeCourseLatencyUpdateAll() override;
        virtual bool HasPendingRelapseInfection() const override;
        virtual bool HasEverRelapsedAfterTreatment() const override;

        //IIndividualHumanHIV
        virtual IInfectionHIV* GetHIVInfection() const override;
        virtual ISusceptibilityHIV* GetHIVSusceptibility() const override;
        virtual IHIVInterventionsContainer* GetHIVInterventionsContainer() const override;
        virtual std::string toString() const override; // serialization, for logging

        //IIndividualHumanTB
        //virtual void RegisterInfectionIncidenceObserver( IInfectionIncidenceObserver *); //move this is not IIndividualHumanTB, not override
        virtual bool HasFailedTreatment() const override;
        virtual bool IsTreatmentNaive() const override;
        virtual bool IsFastProgressor() const override;
        virtual bool IsOnTreatment() const override;
        virtual bool IsEvolvedMDR() const override;
        virtual float GetDurationSinceInitInfection() const override;

        IInfectionTB* GetTBInfection() const ; 
        //virtual void UnRegisterAllObservers ( IInfectionIncidenceObserver *); // does not override

        //used for ReportTBHIV
        virtual bool HasHIV() const override;
        virtual NaturalNumber GetViralLoad() const override;
        virtual const std::list< Susceptibility* > &GetSusceptibilityList() override;
        virtual ITBInterventionsContainer* GetTBInterventionsContainer() const override;

        //IIndividualHumanContext
        virtual IIndividualHumanInterventionsContext* GetInterventionsContextbyInfection(Infection* infection) override;
        virtual float GetNextLatentActivation(float time) const override;

        //Hooks for changing infectiousness, activation, mortality based on CD4 count and age (maybe there is a more natural spot for this)
        virtual void ModActivate() override;
        virtual void SetTBActivationVector(const std::vector <float>& vin) override;
        virtual const std::vector <float>& GetTBActivationVector() const override;

        // CoInfection reactivation
        std::vector <float> CD4_forward_vector;
        std::vector<float> TB_activation_vector;

        virtual float GetTBCD4InfectiousnessMap( float ) const override;
        virtual float GetCD4SusceptibilityMap( float ) const override;
        virtual float GetCD4PrimaryMap(float) const override;
        virtual bool GetExogenousTBStateChange() const override;
        
        //counter
        void ResetCounters( void );
        float infectionMDRIncidenceCounter;
        float mdr_evolved_incident_counter;
        float new_mdr_fast_active_infection_counter;

        virtual float GetImmunityReducedAcquire() override;

    protected:
        IndividualHumanCoInfection( suids::suid _suid, float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0);

        // Factory methods
        virtual Infection* createInfection(suids::suid _suid);
        virtual bool createInfection( suids::suid _suid, infection_list_t &newInfections );
        virtual void setupInterventionsContainer();
        virtual bool SetNewInfectionState(InfectionStateChange::_enum inf_state_change);

        //infection_list_t newInfectionlist;
        std::list< Susceptibility* > susceptibilitylist;

        std::map< IInfection*, Susceptibility*> infection2susceptibilitymap;
        //std::map< IInfection*, InterventionsContainer*> infection2interventionsmap;

        //future, please use the list not the individual ones
        Susceptibility* susceptibility_tb;
        Susceptibility* susceptibility_hiv;

        //infection_list_t infectionslist;
        int infectioncount_tb;
        int infectioncount_hiv;

        bool m_has_ever_been_onART;
        bool m_has_ever_tested_positive_for_HIV;
        bool m_bool_exogenous;

        //event observers
        //std::vector < IInfectionIncidenceObserver * > infectionIncidenceObservers; 
        static void InitializeStaticsCoInfection( const Configuration* config );

        //Strain tracking
        virtual bool InfectionExistsForThisStrain(IStrainIdentity* check_strain_id);
        void SetExogenousInfectionStrain(IStrainIdentity* exog_strain_id);

        virtual void PropagateContextToDependents() override;


    private:
        virtual IIndividualHumanContext* GetContextPointer( ) override { return (IIndividualHumanContext*)this; };
        DECLARE_SERIALIZABLE(IndividualHumanCoInfection);
    };
}

