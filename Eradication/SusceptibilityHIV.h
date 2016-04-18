/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "SusceptibilitySTI.h"

#include "Types.h"

namespace Kernel
{
    struct IIndividualHumanHIV;
    class IInfectionHIV;
    class ISusceptibilityHIV : public ISupports
    {
    public:
        // disease specific functions go here
        virtual float        GetCD4count() const = 0;
        virtual void         Generate_forward_CD4() = 0;
        virtual void         FastForward( const IInfectionHIV * const, float dt ) = 0;
        virtual void         ApplyARTOnset() = 0;
        virtual ProbabilityNumber GetPrognosisCompletedFraction() const = 0;
        virtual void         TerminateSuppression(float days_till_death) = 0;
    };

    class SusceptibilityHIVConfig : public JsonConfigurable
    {
        friend class IndividualHumanCoinfection;
        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityHIVConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        virtual bool Configure( const Configuration* config ) override;

    protected:
        //these are the config params
        static float post_infection_CD4_lambda;
        static float post_infection_CD4_inverse_kappa;
        static float disease_death_CD4_alpha;
        static float disease_death_CD4_inverse_beta;
    };

    //---------------------------- SusceptibilityHIV ----------------------------------------
    class SusceptibilityHIV : public SusceptibilitySTI, virtual public ISusceptibilityHIV, public SusceptibilityHIVConfig
    {
    public:
        friend class IndividualHumanCoinfection;
         friend class IndividualHumanHIV;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
        //virtual bool Configure( const Configuration* config );

        virtual ~SusceptibilityHIV(void);
        static Susceptibility *CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod);

        void SetContextTo(IIndividualHumanContext* context) override;
        virtual void Update(float dt = 0.0) override;
        virtual void UpdateInfectionCleared() override;

        // disease specific functions 
        virtual float GetCD4count() const override;
        virtual void  Generate_forward_CD4() override;
        virtual void  FastForward( const IInfectionHIV * const, float dt ) override;
        virtual void  ApplyARTOnset() override;
        virtual ProbabilityNumber GetPrognosisCompletedFraction() const override;
        virtual void  TerminateSuppression(float days_till_death) override;

    protected:
        //disease specific params 
        // static float TB_immune_loss_fraction;
        SusceptibilityHIV();
        SusceptibilityHIV(IIndividualHumanContext *context);
        void setCD4Rate(const IInfectionHIV * const pInf);
        IIndividualHumanHIV * hiv_parent;

        /* clorton virtual */ void Initialize(float age, float immmod, float riskmod) /* clorton override */;
        void UpdateSymptomaticPresentationTime();

        // additional members of SusceptibilityHIV (params)
        float days_between_symptomatic_and_death;   // Days before death to broadcast HIVSymptomatic

        float sqrtCD4_Current;          // Current sqrt( CD4count )
        float sqrtCD4_Rate;             // Rate of change of sqrt_CD4count = (sqrt_FinalCD4 - sqrt_InitialCD4)/ Prognosis
        float sqrtCD4_PostInfection;    // sqrt of CD4 count after acute drop
        float sqrtCD4_AtDiseaseDeath;   // sqrt of CD4 count at HIV-cause death

        float CD4count_at_ART_start;    // CD4 count at start of ART, if any

        DECLARE_SERIALIZABLE(SusceptibilityHIV);
    };
}
