/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "BoostLibWrapper.h"
#include "Common.h"
#include "IMalariaAntibody.h" // for MalariaAntibodyType enum, containers of IMalariaAntibody pointers
#include "SimpleTypemapRegistration.h"
#include "MalariaEnums.h"
#include "MalariaContexts.h"
#include "SusceptibilityVector.h"
#include "InterventionEnums.h"

namespace Kernel
{
    class SusceptibilityMalariaConfig : public JsonConfigurable 
    {
        friend class IndividualHumanMalaria;

    public:
        virtual bool Configure( const Configuration* config );

        // These public configurable parameters are accessed by MalariaAntibody for Decay and Update functions
        static float memory_level;
        static float hyperimmune_decay_rate;
        static float MSP1_antibody_growthrate;
        static float antibody_stimulation_c50;
        static float antibody_capacity_growthrate;
        static float minimum_adapted_response;
        static float non_specific_growth;
        static float antibody_csp_decay_days;

    protected:
        static MaternalAntibodiesType::Enum maternal_antibodies_type;
        static float maternal_antibody_protection;
        static float maternal_antibody_decay_rate;
        static bool sexual_combination;
        static InnateImmuneVariationType::Enum innate_immune_variation_type;
        static float base_gametocyte_mosquito_survival;
        static float cytokine_gametocyte_inactivation;
        static double anemiaMortalityLevel;
        static double parasiteMortalityLevel;
        static double feverMortalityLevel;
        static float anemiaMortalityInvWidth;
        static float parasiteMortalityInvWidth;
        static float feverMortalityInvWidth;
        static float anemiaSevereLevel;
        static float parasiteSevereLevel;
        static float feverSevereLevel;
        static float anemiaSevereInvWidth;
        static float parasiteSevereInvWidth;
        static float feverSevereInvWidth;
        static float erythropoiesis_anemia_effect;
        static float pyrogenic_threshold;
        static float fever_IRBC_killrate;

        // thresholds for defining a unique clinical incident (from spiking symptoms of concurrent infections)
        // EAW: are either of these redundant with IndividualHumanFlagsMalaria.feverDetectionThreshold?
        static float clinicalFeverThreshold_low;
        static float clinicalFeverThreshold_high;
        static float minDaysBetweenClinicalIncidents;

        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityMalariaConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    };

    class SusceptibilityMalaria : public SusceptibilityVector, public IMalariaSusceptibility, protected SusceptibilityMalariaConfig
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:

        static SusceptibilityMalaria *CreateSusceptibility(IIndividualHumanContext *context, float _age = 20 * DAYSPERYEAR, float immmod = 1.0f, float riskmod = 1.0f);
        virtual ~SusceptibilityMalaria();

        virtual void Update(float dt);
        virtual void UpdateInfectionCleared();

        // functions to mediate interaction with Infection_Malaria objects
        virtual void  UpdateActiveAntibody( pfemp1_antibody_t &pfemp1_variant, int minor_variant, int major_variant );

        // IMalariaSusceptibility interfaces
        virtual void   SetAntigenPresent();
        virtual float  get_fever()              const;
        virtual float  get_fever_celsius()      const;
        virtual float  get_cytokines()          const;
        virtual double get_RBC_availability()   const;
        virtual float  get_parasite_density()   const;
        virtual float  GetMaxFever()            const;
        virtual float  GetMaxParasiteDensity()  const;
        virtual float  GetHemoglobin()          const;
        virtual bool   CheckForParasitesWithTest( int test_type = MALARIA_TEST_BLOOD_SMEAR ) const;
        virtual float  CheckParasiteCountWithTest( int test_type = MALARIA_TEST_BLOOD_SMEAR ) const;
        virtual float  get_fraction_of_variants_with_antibodies(MalariaAntibodyType::Enum type) const;
        virtual IMalariaAntibody* RegisterAntibody(MalariaAntibodyType::Enum type, int variant, float capacity=0.0f);
        virtual SevereCaseTypesEnum::Enum  CheckSevereCaseType() const;
        virtual float  get_inv_microliters_blood() const;
        virtual void   ResetMaximumSymptoms();
        virtual long long get_RBC_count()             const;
        virtual float  get_maternal_antibodies() const;
        virtual void   init_maternal_antibodies(float mother_factor);
        virtual float  get_fever_killing_rate() const;

        // functions to mediate interactions with red blood cell count
        virtual void   remove_RBCs(int64_t infectedAsexual, int64_t infectedGametocytes, double RBC_destruction_multiplier);

    protected:

        const SimulationConfig *params() const;

        // Functions to enforce antigen-antibody reactions (e.g. stimulation, decay)
        void  updateImmunityCSP( float dt );
        void  updateImmunityMSP( float dt, float& temp_cytokine_stimulation );
        void  updateImmunityPfEMP1Minor( float dt );
        void  updateImmunityPfEMP1Major( float dt );
        void  decayAllAntibodies( float dt );
        void  recalculateBloodCapacity( float _age );
        void  countAntibodyVariations();

        // Immune initialization
        void InitializeAntibodyVariants(MalariaAntibodyType::Enum type, float frac_variants);
        std::vector<int> InitialVariants(int n_choose, int n_total);

        // IAntibodyBoostable functions
        virtual void BoostAntibody( MalariaAntibodyType::Enum type, int variant, float boosted_antibody_concentration );

        // Clinical outcome calculations
        void  updateClinicalStates( float dt );
        void  ReportClinicalCase( ClinicalSymptomsEnum::Enum symptom );
        float get_severe_disease_probability( float dt, float& anemiaSevereFraction, float& parasiteSevereFraction, float& feverSevereFraction );
        float get_fatal_disease_probability(  float dt, float& anemiaFatalFraction,  float& parasiteFatalFraction,  float& feverFatalFraction );

        // Calculates total probability and adjusts fractional probabilities by cause
        float get_combined_probability ( 
            float dt, 
            float anemiaThreshold, float anemiaInvWidth,
            float parasiteThreshold, float parasiteInvWidth,
            float feverThreshold, float feverInvWidth,
            float& anemiaPartialProb, float& parasitePartialProb, float& feverPartialProb);

        void  UpdateMaximumSymptoms();

        int32_t m_antigenic_flag;

        // containers for antibody objects
        float m_maternal_antibody_strength;
        IMalariaAntibody* m_CSP_antibody;
        std::vector<IMalariaAntibody*> m_active_MSP_antibodies;
        std::vector<IMalariaAntibody*> m_active_PfEMP1_minor_antibodies;
        std::vector<IMalariaAntibody*> m_active_PfEMP1_major_antibodies;

        // RBC information
        int64_t m_RBC;
        int64_t m_RBCcapacity;
        int64_t m_RBCproduction;   // how many RBC's a person should have /120 (AVERAGE_RBC_LIFESPAN)
        float   m_inv_microliters_blood; // ==/(age dependent estimate of blood volume)

        // symptomatic variables
        float m_cytokines;
        float m_ind_pyrogenic_threshold;
        float m_ind_fever_kill_rate;
        float m_cytokine_stimulation;
        float m_parasite_density;
        std::vector<int> m_antibodies_to_n_variations; // how many variations of each antigen type does an individual have antibodies to

        // maxima over sub-time-step updates.  reset every time step.
        float m_max_fever_in_tstep; 
        float m_max_parasite_density_in_tstep;
        SevereCaseTypesEnum::Enum severetype;

        // state variables for whether the individual is in clinical disease, severe disease, or dead
        float  cumulative_days_of_clinical_incident;
        float  cumulative_days_of_severe_incident;
        float  cumulative_days_of_severe_anemia_incident;
        float  days_between_incidents;

    private:

        SusceptibilityMalaria();
        SusceptibilityMalaria(IIndividualHumanContext *context);
        void Initialize(float _age, float immmod, float riskmod);

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, SusceptibilityMalaria& sus, const unsigned int file_version );
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
     // IJsonSerializable Interfaces
     virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
     virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };
}
