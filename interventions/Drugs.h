/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IDrug.h"
#include "InterventionEnums.h"      // return types
#include "InterventionFactory.h"    // macros that 'auto'-register classes
#include "Configure.h"              // base classes
#include "SimulationEnums.h"        // for PkPdModel

namespace Kernel
{
    struct ICampaignCostObserver;
    struct IMalariaDrugEffectsApply;

    class GenericDrug : public IDrug, public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, GenericDrug, IDistributableIntervention)

    public:
        virtual ~GenericDrug();

        virtual bool Configure( const Configuration * ) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual int AddRef() override;
        virtual int Release() override;

        // IDistributableIntervention
        virtual void Update(float dt) override;
        virtual bool NeedsInfectiousLoopUpdate() const
        { 
            // Drugs typically change things that affect the infection so
            // they need to be in the infectious update loop.
            return true;
        }
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;

        // IDrug
        virtual const std::string& GetDrugName() const override;
        virtual float GetDrugCurrentEfficacy() const override;

    protected:
        GenericDrug( const std::string& rDefaultName = JsonConfigurable::default_string );

        virtual bool IsTakingDose( float dt ) { return true; }

        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc );
        virtual float GetDrugReducedAcquire()  const;
        virtual float GetDrugReducedTransmit() const;
        virtual float GetDrugCurrentConcentration() const;

        // interfaces for different PkPd models (i.e. FIXED_DURATION_CONSTANT_EFFECT, CONCENTRATION_VERSUS_TIME)
        virtual void SimpleUpdate(float dt);
        virtual void UpdateWithPkPd(float dt);

        virtual void ResetForNextDose(float dt);
        virtual void ApplyEffects(); // virtual, not part of interface
        virtual void PkPdParameterValidation();
        virtual void Expire();

        float CalculateEfficacy( float c50, float startConcentration, float endConcentration );


        std::string drug_name;
        PKPDModel::Enum durability_time_profile;
        float fast_decay_time_constant;
        float slow_decay_time_constant;
        float dosing_timer;        //time to next dose for ongoing treatment
        int   remaining_doses;     //number of doses left in treatment
        float time_between_doses;
        float fast_component;
        float slow_component;

        float start_concentration;
        float end_concentration;
        float current_concentration;
        float current_efficacy;
        float current_reducedacquire;
        float current_reducedtransmit;
        float pk_rate_mod;
        float Cmax;
        float Vd;
        float drug_c50;
        float fraction_defaulters;

        DECLARE_SERIALIZABLE(GenericDrug);
    };
}
