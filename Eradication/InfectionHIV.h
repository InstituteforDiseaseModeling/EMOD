
#pragma once
#include "InfectionSTI.h"
#include "IInfectionHIV.h"
#include "SusceptibilityHIV.h"
#include "FerrandAgeDependentDistribution.h"
#include "HIVEnums.h"
#include "Types.h"

namespace Kernel
{
    class InfectionHIVConfig : public InfectionSTIConfig
    {
        friend class HIVInterventionsContainer;

        GET_SCHEMA_STATIC_WRAPPER(InfectionHIVConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        virtual bool Configure( const Configuration* config ) override;

    protected:
        friend class InfectionHIV;
        friend class IndividualHumanCoInfection;

        //these are the config params
        static float HIV_drug_inactivation_rate;
        static float HIV_drug_clearance_rate;
        static float acute_duration_in_months;
        static float AIDS_duration_in_months;
        static float acute_stage_infectivity_multiplier;
        static float AIDS_stage_infectivity_multiplier;
        static float personal_infectivity_heterogeneity;
        static float personal_infectivity_scale;
        static FerrandAgeDependentDistribution mortality_distribution_by_age;
        static IDistribution* p_hetero_infectivity_distribution;
    };

    //---------------------------- InfectionHIV ----------------------------------------
    class InfectionHIV : public InfectionSTI, public IInfectionHIV
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        virtual ~InfectionHIV(void);
        static InfectionHIV *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);

        virtual void SetParameters( const IStrainIdentity* infstrain, int incubation_period_override = -1 ) override;
        virtual void Update( float currentTime, float dt, ISusceptibilityContext* immunity = nullptr ) override;
        virtual void SetContextTo(IIndividualHumanContext* context) override;

        virtual NaturalNumber GetViralLoad() const override;
        virtual float GetPrognosis() const override;
        virtual float GetTimeInfected() const override;
        virtual float GetDaysTillDeath() const override;
        virtual float GetInfectiousness() const override;
        virtual void SetupSuppressedDiseaseTimers( float durationFromEnrollmentToArtAidsDeath ) override;
        virtual void ApplySuppressionDropout() override;
        virtual void ApplySuppressionFailure() override;
        
        // kto moved to public for access through interventions
        float GetWHOStage() const override;
        virtual const HIVInfectionStage::Enum& GetStage() const override;

    protected:
        InfectionHIV();
        InfectionHIV(IIndividualHumanContext *context);

        virtual void Initialize(suids::suid _suid);

        void SetupNonSuppressedDiseaseTimers();
        /* clorton virtual */ bool  ApplyDrugEffects(float dt, ISusceptibilityContext* immunity = nullptr);
        void SetStageFromDuration();
        void SetStage( HIVInfectionStage::Enum stage );

        void SetupWouldHaveTimers();
        void UpdateWouldHaveTimers( float dt );

        // additional infection members
        float ViralLoad;
        float HIV_duration_until_mortality_without_TB;
        float HIV_natural_duration_until_mortality;
        float HIV_duration_until_mortality_with_viral_suppression;

        float m_time_infected;

        HIVInfectionStage::Enum m_infection_stage;

        float m_fraction_of_prognosis_spent_in_stage[ NUM_WHO_STAGES ];

        float m_acute_duration; // Measured in days
        float m_latent_duration;
        float m_aids_duration;

        float m_duration_until_would_have_entered_latent;
        float m_duration_until_would_have_entered_aids;
        float m_duration_until_would_have_died;
        bool m_would_have_entered_latent;
        bool m_would_have_entered_aids;
        bool m_would_have_died;
        bool m_has_been_suppressed;

        float m_hetero_infectivity_multiplier;

        virtual void UpdateSymptomatic( float const duration, float const incubation_timer ) override {};
        virtual bool IsSymptomatic() const override { return false; };


        IIndividualHumanHIV * hiv_parent;

        DECLARE_SERIALIZABLE(InfectionHIV);
    };
}
