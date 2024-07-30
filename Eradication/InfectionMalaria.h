
#pragma once

#include <utility>
#include "InfectionVector.h"

#include "MalariaContexts.h"
#include "MalariaEnums.h"

namespace Kernel
{
    class MalariaAntibody;
    struct IMalariaDrugEffects;

    class InfectionMalariaConfig : public InfectionVectorConfig
    {
        GET_SCHEMA_STATIC_WRAPPER(InfectionMalariaConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        InfectionMalariaConfig() {};
        virtual bool Configure( const Configuration* config ) override;
        virtual void ConfigureMalariaStrainModel( const Configuration* config );

    protected:
        friend class InfectionMalaria;
        
        static ParasiteSwitchType::Enum parasite_switch_type;
        static MalariaStrains::Enum     malaria_strains;

        static float antibody_IRBC_killrate;
        static float MSP1_merozoite_kill;
        static float gametocyte_stage_survival;
        static float base_gametocyte_sexratio;
        static float base_gametocyte_production;
        static float antigen_switch_rate;
        static float merozoites_per_hepatocyte;
        static float merozoites_per_schizont;
        static float non_specific_antigenicity;
        static float RBC_destruction_multiplier;
        static int    n_asexual_cycles_wo_gametocytes;
    };

    class InfectionMalaria : public InfectionVector, public IInfectionMalaria
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static const std::vector<uint32_t> STRIDE_LENGTHS;

        static InfectionMalaria *CreateInfection( IIndividualHumanContext *context, suids::suid suid, int initial_hepatocytes=1 );
        virtual ~InfectionMalaria();

        // TODO - becomes part of initialize?
        virtual void SetParameters( const IStrainIdentity* _infstrain, int incubation_period_override = -1 ) override;
        virtual void InitInfectionImmunology(ISusceptibilityContext* _immunity) override;

        virtual void Update( float currentTime, float dt, ISusceptibilityContext* immunity = nullptr ) override;

        // TODO: intrahost_report needs to be reimplemented as a custom reporter

        // Sums up the current parasite counts for the infection and determines if the infection is cleared or if death occurs this time step
        void malariaCheckInfectionStatus( float dt, IMalariaSusceptibility * immunity );

        // Calculates the IRBC killing from drugs and immune action
        void malariaImmunityIRBCKill( float dt, IMalariaSusceptibility * immunity );

        // Calculates stimulation of immune system by malaria infection
        void malariaImmuneStimulation( float currentTime, float dt, IMalariaSusceptibility * immunity );

        // Calculates immature gametocyte killing from drugs and immune action
        void malariaImmunityGametocyteKill( float dt, IMalariaSusceptibility * immunity );

        // Calculates the antigenic switching when an asexual cycle completes and creates next generation of IRBC's
        void malariaIRBCAntigenSwitch(double = 1.0);

        // Moves all falciparum gametocytes forward a development stage when an asexual cycle completes, and creates the stage 0 immature gametocytes
        void malariaCycleGametocytes(double = 1.0);

        // Process all infected hepatocytes
        void malariaProcessHepatocytes( float dt, IMalariaSusceptibility * immunity );

        virtual int32_t get_hepatocytes() const override { return m_hepatocytes;  }
        virtual int64_t get_MaleGametocytes(int stage) const override;
        virtual int64_t get_FemaleGametocytes(int stage) const override;
        virtual void apply_MatureGametocyteKillProbability(float pkill) override;

        virtual float get_asexual_density() const override;
        virtual float get_mature_gametocyte_density() const override;
        virtual int64_t get_irbc() const override;
        virtual MalariaInfectionStage::Enum get_InfectionStage() const override;
        virtual bool did_InfectionStageChangeToday() const override;

        virtual void SetContextTo(IIndividualHumanContext* context) override;

    protected:
        void processEndOfAsexualCycle( float currentTime, float dt, IMalariaSusceptibility* immunity );
        
        int64_t CalculateTotalIRBC() const;
        virtual int64_t CalculateTotalIRBCWithHRP( int64_t totalIRBC ) const;


        InfectionMalaria();
        InfectionMalaria(IIndividualHumanContext *context);
        void Initialize(suids::suid suid, int initial_hepatocytes);
        std::string ValidateGametocyteCounts() const;
        std::string ValidateIRBCCounts() const;


        // duration, incubation period, and infectious period are reused with different meanings from Infection, and Infection_Vector
        double m_IRBCtimer;
        int32_t m_hepatocytes;
        AsexualCycleStatus::Enum m_asexual_phase;
        int32_t m_asexual_cycle_count;

        int32_t m_MSPtype;        // allow variation in MSP from clone to clone
        int32_t m_nonspectype;    // what is the set of minor_epitope_types
        int32_t m_minor_epitope_type[CLONAL_PfEMP1_VARIANTS]; // TODO: these could be vectors that get cleared after setting up antibodies in InitInfectionImmunology...
        int32_t m_IRBCtype[CLONAL_PfEMP1_VARIANTS];

        // Refactored antigen-antibody interaction.  Keep pointers to antibody objects instead of variant indices.
        MalariaAntibody* m_MSP_antibody; // TODO: would be nice to protect this from changing (const pointer) but it is only set in InitInfectionImmunology        
        std::vector< pfemp1_antibody_t > m_PfEMP1_antibodies;

        std::vector<int64_t> m_IRBC_count;
        int64_t m_malegametocytes[GametocyteStages::Count];
        int64_t m_femalegametocytes[GametocyteStages::Count];

        // govern distribution of next merozoites
        double m_gametorate;
        double m_gametosexratio;

        double m_measured_duration;
        bool m_start_measuring;
        double m_temp_duration;
        int64_t m_max_parasites;
        double m_inv_microliters_blood;   // tracks blood volume based on age

        IMalariaDrugEffects* m_pMDE;

        MalariaInfectionStage::Enum m_CurrentInfectionStage;
        float m_TimeSinceInfectionStageChange;


        DECLARE_SERIALIZABLE(InfectionMalaria);
    };
}
