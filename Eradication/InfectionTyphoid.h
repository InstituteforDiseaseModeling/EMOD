/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "InfectionEnvironmental.h"
#include "TyphoidDefs.h" // for N_TYPHOID_SEROTYPES
#include "Timers.h" // for N_TYPHOID_SEROTYPES

namespace Kernel
{
    class InfectionTyphoidConfig : public InfectionEnvironmentalConfig
    {
        friend class IndividualTyphoid;
        GET_SCHEMA_STATIC_WRAPPER(InfectionTyphoidConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        virtual bool Configure( const Configuration* config ) override;
        
        static IDistribution* p_log_normal_distribution;
    };

    class IInfectionTyphoid : public ISupports
    {
        public:
        virtual void Clear() = 0;
        virtual const std::string& GetStateToReport() const = 0;
        virtual bool  IsStateChanged() const = 0;
    };

    class InfectionTyphoid : public InfectionEnvironmental, public IInfectionTyphoid 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static InfectionTyphoid *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);
        virtual ~InfectionTyphoid(void);

        virtual void SetParameters( IStrainIdentity* infstrain = NULL, int incubation_period_override = -1 ) override;
        virtual void InitInfectionImmunology(ISusceptibilityContext* _immunity) override;
        virtual void Update(float dt, ISusceptibilityContext* _immunity = NULL) override;
        void SetMCWeightOfHost(float ind_mc_weight);
        virtual void Clear() override;
        virtual float GetInfectiousness() const override;
        virtual const std::string& GetStateToReport() const override { return state_to_report; }
        virtual bool  IsStateChanged() const override { return state_changed; }

        // InfectionTyphoidReportable methods
    protected:
        InfectionTyphoid(); 

        const SimulationConfig* params();

        InfectionTyphoid(IIndividualHumanContext *context);
        void Initialize(suids::suid _suid);
        void handlePrepatentExpiry();
        void handleAcuteExpiry();
        void handleSubclinicalExpiry();
        float treatment_multiplier;
        int chronic_timer;
        int chronic_timer_2;
        int subclinical_timer;
        int acute_timer;
        int treatment_timer;
        int treatment_day;
        //int prepatent_timer;
        int prepatent_timer;
        int clinical_immunity_timer;  // timers of days left in state, or UNINIT_TIMER if not used //JG- I'm going to leave clinical immunity in for now. 
        float _subclinical_duration;
        float _prepatent_duration;
        float _acute_duration; // duration of state in days
        bool isDead;  // is this individual dead?
        std::string state_to_report; // default state is susceptible
        std::string last_state_reported; // previous typhoid status of individual
        bool state_changed;
        static const int _chronic_duration;
        static const int _clinical_immunity_duration;

        static const int acute_treatment_day; // what day individuals are treated
        static const float CFRU;   // case fatality rate?
        static const float CFRH; // hospitalized case fatality rate?
        static const float treatmentprobability;  // probability of treatment
    

        // Incubation period by transmission route (taken from Glynn's dose response analysis) assuming low dose for environmental.
        // mean and std dev of log normal distribution
        static const float mph;
        static const float sph;
        static const float mpm;
        static const float spm;
        static const float mpl;
        static const float spl;

        // Subclinical infectious duration parameters: mean and standard deviation under and over 30
        static const float mso30;
        static const float sso30;
        static const float msu30;
        static const float ssu30;

        // Acute infectious duration parameters: mean and standard deviation under and over 30
        static const float mao30;
        static const float sao30;
        static const float mau30;
        static const float sau30;

        static const float P10; // probability of clinical immunity from a subclinical infection

    private:
        DECLARE_SERIALIZABLE(InfectionTyphoid);
    };
}

