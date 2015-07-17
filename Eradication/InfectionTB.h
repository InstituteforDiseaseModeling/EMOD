/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#ifdef ENABLE_TB
#include "InfectionAirborne.h"

#include "TBInterventionsContainer.h"
#include "InterventionEnums.h"
#include "SusceptibilityTB.h"
#include "Infection.h"
#include "Interventions.h"

namespace Kernel
{
    // find a home for these...  TBEnums.h?
    ENUM_DEFINE(TBInfectionDrugResistance,
        ENUM_VALUE_SPEC(DrugSensitive           , 0)
        ENUM_VALUE_SPEC(FirstLineResistant      , 1))
    class IIndividualHumanCoinfection;

    class IInfectionTB : public ISupports
    {
    public:
        virtual bool IsSmearPositive() const = 0;
        virtual bool IsMDR() const = 0 ; 
        virtual void SetIncubationTimer(float) = 0;
        virtual float GetLatentCureRate() const = 0;
        virtual void ResetRecoverFraction(float) = 0;
        virtual void ResetDuration() = 0;
        virtual bool IsSymptomatic() const = 0;
        virtual bool IsActive() const = 0;
        virtual bool IsExtrapulmonary() const = 0; 

    };


    class InfectionTBConfig : public JsonConfigurable
    {
        friend class IndividualTB;
        GET_SCHEMA_STATIC_WRAPPER(InfectionTBConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        bool Configure( const Configuration* config );
        std::map <float,float> GetCD4Map();
        InfectionStateChange::_enum TB_event_type_associated_with_infectious_timer;
        

    protected:
        static float TB_latent_cure_rate;
        static float TB_fast_progressor_rate;
        static float TB_slow_progressor_rate;
        static float TB_active_cure_rate;
        static float TB_inactivation_rate;
        static float TB_active_mortality_rate;
        static float TB_extrapulmonary_mortality_multiplier;
        static float TB_smear_negative_mortality_multiplier;
        static float TB_active_presymptomatic_infectivity_multiplier;
        static float TB_presymptomatic_rate;
        static float TB_presymptomatic_cure_rate;
        static float TB_smear_negative_infectivity_multiplier;
        static float TB_Drug_Efficacy_Multiplier_MDR;
        static float TB_Drug_Efficacy_Multiplier_Failed;
        static float TB_Drug_Efficacy_Multiplier_Relapsed;
        static float TB_MDR_Fitness_Multiplier;
        static std::map <float,float> CD4_map;
        static float TB_relapsed_to_active_rate;
        
        static DistributionFunction::Enum TB_active_period_distribution;
        static float TB_active_period_std_dev;

        static vector <float> TB_cd4_activation_vec;
        static vector <float> CD4_strata_act_vec;

    };

    //---------------------------- InfectionTB ----------------------------------------
    class InfectionTB : public InfectionAirborne, public IInfectionTB, protected InfectionTBConfig
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        virtual ~InfectionTB(void);
        static InfectionTB *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);

        virtual void SetParameters(StrainIdentity* infstrain=NULL, int incubation_period_override = -1);
        virtual void Update(float dt, Susceptibility* immunity = NULL);
        virtual void InitInfectionImmunology(Susceptibility* _immunity);

        // Inherited from base class
        virtual bool IsActive() const;

        //TB-specific
        virtual bool IsSmearPositive() const;
        virtual bool IsExtrapulmonary() const;
        bool IsFastProgressor() const;
        virtual bool IsMDR() const; 
        bool EvolvedResistance() const;
        bool IsPendingRelapse() const;
        virtual bool IsSymptomatic() const;
        virtual void SetIncubationTimer (float new_timer);
        virtual float GetLatentCureRate() const ;
        virtual void ResetRecoverFraction(float new_fraction);
        virtual void ResetDuration();
        float GetDurationSinceInitialInfection() const; 

    protected:
        InfectionTB();
        InfectionTB(IIndividualHumanContext *context);

        // For disease progression and MDR evolution, virtual functions are inherited from base class Infection
        virtual void  Initialize(suids::suid _suid);
        void  InitializeLatentInfection(Susceptibility* immunity);
        void  InitializeActivePresymptomaticInfection(Susceptibility* immunity);
        void  InitializeActiveInfection(Susceptibility* immunity);
        void  InitializePendingRelapse(Susceptibility* immunity);
        //const SimulationConfig* params();
        bool  ApplyDrugEffects(float dt, Susceptibility* immunity = NULL);
        virtual void EvolveStrain(Susceptibility* _immunity, float dt);
        TBDrugEffects_t GetTotalDrugEffectsForThisInfection();
        float CalculateTimerAgeDepSlowProgression(Susceptibility* immunity);

        // additional TB infection members
        // This chunk gets serialized.
        IIndividualHumanCoinfection* human_coinf; //  = NULL;
        bool  m_is_active;
        float m_recover_fraction;
        float m_death_fraction;
        bool  m_is_smear_positive;
        bool  m_is_extrapulmonary;
        bool  m_is_fast_progressor;
        bool  m_evolved_resistance;
        bool  m_is_pending_relapse;
        bool  m_shows_symptoms;
        float m_duration_since_init_infection; //for reporting only

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, InfectionTB& inf, const unsigned int file_version );
#endif
    };
}
#endif
