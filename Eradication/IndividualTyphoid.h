/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "InfectionTyphoid.h"
#include "IndividualEnvironmental.h"
#include "SusceptibilityTyphoid.h"

namespace Kernel
{
    class SusceptibilityTyphoid;
    class IndividualHumanTyphoidConfig : public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(IndividualHumanTyphoidConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    protected:
        friend class SimulationTyphoid;
        friend class IndividualHumanTyphoid;
        friend class InfectionTyphoid;
        friend class SusceptibilityTyphoid;
        friend class NodeTyphoid;
        static float environmental_incubation_period; // NaturalNumber please
        static float typhoid_acute_infectiousness;
        static float typhoid_chronic_relative_infectiousness;

        static float typhoid_prepatent_relative_infectiousness;
        static float typhoid_protection_per_infection;
        static float typhoid_subclinical_relative_infectiousness;
        static float typhoid_carrier_probability;
        static float typhoid_3year_susceptible_fraction;
        static float typhoid_6month_susceptible_fraction;
        static float typhoid_6year_susceptible_fraction;
        static float typhoid_symptomatic_fraction;

        static float typhoid_environmental_exposure_rate;
        static float typhoid_contact_exposure_rate;
        static float typhoid_environmental_peak_multiplier;
        static float typhoid_exposure_lambda;

        virtual bool Configure( const Configuration* config );
    };


    class IIndividualHumanTyphoid : public ISupports
    {
    public:
        virtual bool IsChronicCarrier( bool incidence_only = true ) const = 0;
        virtual bool IsSubClinical( bool incidence_only = true ) const = 0;
        virtual bool IsAcute( bool incidence_only = true ) const = 0;
        virtual bool IsPrePatent( bool incidence_only = true ) const = 0;
        virtual void ForceClearInfection() = 0;
    };

    class IndividualHumanTyphoid : public IndividualHumanEnvironmental, public IIndividualHumanTyphoid
    {
        friend class SimulationTyphoid;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE()
        DECLARE_SERIALIZABLE(IndividualHumanTyphoid);

    public:
        static IndividualHumanTyphoid *CreateHuman(INodeContext *context, suids::suid id, float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0);
        virtual ~IndividualHumanTyphoid(void);
        static void InitializeStatics( const Configuration* config );

        virtual void CreateSusceptibility(float imm_mod = 1.0, float risk_mod = 1.0);

        virtual bool IsChronicCarrier( bool incidence_only = true ) const;
        virtual bool IsSubClinical( bool incidence_only = true ) const;
        virtual bool IsAcute( bool incidence_only = true ) const;
        virtual bool IsPrePatent( bool incidence_only = true ) const;
        const std::string getDoseTracking() const;

        // New Exposure Pattern
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route );

        IndividualHumanTyphoid(suids::suid id = suids::nil_suid(), float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0);
        virtual void setupInterventionsContainer();
        virtual void PropagateContextToDependents();

        virtual void UpdateInfectiousness(float dt);
        virtual void Update(float currenttime, float dt);
        virtual Infection* createInfection(suids::suid _suid);
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain = nullptr, int incubation_period_override = -1) override;
        virtual float GetImmunityReducedAcquire() const override;
        virtual HumanStateChange GetStateChange() const;
        //virtual void applyNewInterventionEffects(float dt);
        virtual void ForceClearInfection();
        
    protected:
        void quantizeContactDoseTracking( float fContact );
        void quantizeEnvironmentalDoseTracking( float fEnvironment );

        std::string processPrePatent( float dt );

        // typhoid infection state
        std::string state_to_report; // typhoid status of individual: cache from infection
        bool isChronic;       // is or will be a chronic carrier (using "Ames" definition)
        int _infection_count;     // number of times infected;
        bool state_changed;
        std::string doseTracking;
        float typhoid_infectiousness;

        // typhoid constants
        static const float P1; // probability that an infection becomes clinical
        static const float P5; // probability of typhoid death
        //////////JG REMOVE static const float P6; // probability of sterile immunity after acute infection
        static const float P7; // probability of clinical immunity after acute infection
        //////////JG REMOVE static const float P8; // probability of sterile immunity from a subclinical infectin in the clinically immune
        //////////JG REMOVE static const float P9; // probability of sterile immunity from a subclinical infection


        // typhoid constants from "OutBase.csv" file
        //////////JG REMOVE static float agechronicmale[200]; //probability of becoming chronic carrier, male
        //////////JG REMOVE static float agechronicfemale[200]; //probability of becoming chronic carrier, female

        // environmental exposure constants
        static const int N50;
        static const float alpha;
        friend class NodeTyphoid;

    private:
        SusceptibilityTyphoid * typhoid_susceptibility;
        std::map< TransmissionRoute::Enum, float > contagion_population_by_route;
    };
}
