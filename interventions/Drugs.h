/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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

        // IDrug
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc = nullptr ) override;
        virtual int   GetDrugType() const override;
        virtual std::string GetDrugName() const override;
        virtual DrugUsageType::Enum GetDrugUsageType() override;
        virtual float GetDrugReducedAcquire()  const override;
        virtual float GetDrugReducedTransmit() const override;
        virtual float GetDrugCurrentConcentration() const override;
        virtual float GetDrugCurrentEfficacy() const override;

    protected:
        GenericDrug();

        // interfaces for different PkPd models (i.e. FIXED_DURATION_CONSTANT_EFFECT, CONCENTRATION_VERSUS_TIME)
        virtual void SimpleUpdate(float dt);
        virtual void UpdateWithPkPd(float dt);

        virtual void ResetForNextDose(float dt);
        virtual void ApplyEffects(); // virtual, not part of interface
        virtual void PkPdParameterValidation();
        virtual void Expire();

        int   drug_type;
        DrugUsageType::Enum dosing_type;
        PKPDModel::Enum durability_time_profile;
        float fast_decay_time_constant;
        float slow_decay_time_constant;
        float dosing_timer;        //time to next dose for ongoing treatment
        int   remaining_doses;     //number of doses left in treatment
        float time_between_doses;
        float fast_component;
        float slow_component;

        float current_efficacy;
        float current_concentration;
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
