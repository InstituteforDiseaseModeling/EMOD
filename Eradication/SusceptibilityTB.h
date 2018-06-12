/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "SusceptibilityAirborne.h"
#include "SimulationEnums.h"
#include "IndividualCoInfection.h"

namespace Kernel
{
    class IndividualHumanCoInfection; // fwd declare coz include suddenly doing nothing in scons builds?
    class SusceptibilityTBConfig: public SusceptibilityAirborneConfig
    {
        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityTBConfig)
        friend class IndividualHumanCoInfection;

    public:
        virtual bool Configure( const Configuration* config ) override;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

    protected:
        friend class SusceptibilityTB;
        
        static float TB_immune_loss_fraction;
        static float TB_fast_progressor_fraction_child;
        static float TB_fast_progressor_fraction_adult;
        static float TB_smear_positive_fraction_child;
        static float TB_smear_positive_fraction_adult;
        static float TB_extrapulmonary_fraction_child;
        static float TB_extrapulmonary_fraction_adult;
        static TBFastProgressorType::Enum TB_fast_progressor_fraction_type;
    };

    
    class ISusceptibilityTB : public ISupports
    {
    public:
        virtual float GetFastProgressorFraction() = 0;
        virtual float GetSmearPositiveFraction() = 0;
        virtual float GetExtraPulmonaryFraction() = 0;
        virtual float GetCoughInfectiousness() = 0;
        virtual bool GetCD4ActFlag() const = 0;
        virtual void SetCD4ActFlag( bool bin) = 0;
        virtual float GetProgressionRiskModulator() const = 0;
        virtual void SetModAcquire( float new_mod_acquire ) =0;
        virtual float GetModAcquire(IndividualHumanCoInfection*) const = 0;
    };

    class SusceptibilityTB : public SusceptibilityAirborne, public ISusceptibilityTB
    {
    public:
        friend class IndividualHumanCoInfection;
        friend class IndividualHumanCoInfection;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

        static SusceptibilityTB *CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod);
        virtual ~SusceptibilityTB(void);

        virtual void Update(float dt = 0.0) override;
        virtual void UpdateInfectionCleared() override;
        virtual float GetFastProgressorFraction() override;
        virtual float GetSmearPositiveFraction() override;
        virtual float GetExtraPulmonaryFraction() override;
        virtual void  InitNewInfection() override;
        virtual bool  IsImmune() const override;
        virtual float GetCoughInfectiousness() override;
        virtual bool GetCD4ActFlag() const override;
        virtual void SetCD4ActFlag(bool bin) override;
        virtual float GetProgressionRiskModulator() const override;
        virtual void SetModAcquire(float new_mod_acquire) override;
        virtual float GetModAcquire(IndividualHumanCoInfection*) const;
        virtual float getModTransmit(IndividualHumanCoInfection*) const ;

    protected:
        bool  Flag_use_CD4_for_act;
        SusceptibilityTB();
        SusceptibilityTB(IIndividualHumanContext *context);

        virtual void Initialize(float age, float immmod, float riskmod) override;

        // additional members of SusceptibilityTB
        bool m_is_immune_competent;
        bool m_is_immune;
        float m_fast_progression_modulator_immmod;

        // keeping count of current infections to know when to begin immunity loss.  
        // TODO: alternatively, could add an interface, ITBHumanContext, with a function checking if infected?
        int  m_current_infections;  
        float m_cough_infectiousness;

        DECLARE_SERIALIZABLE(SusceptibilityTB);
    };
}
