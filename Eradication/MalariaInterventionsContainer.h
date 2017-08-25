/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>

#include "VectorInterventionsContainer.h"
#include "MalariaContexts.h"

namespace Kernel
{
    // this container becomes a help implementation member of the relevant IndividualHuman class 
    // it needs to implement consumer interfaces for all the relevant intervention types
    struct IMalariaDrugEffects : public ISupports
    {
        virtual float get_drug_IRBC_killrate() = 0;
        virtual float get_drug_hepatocyte() = 0;
        virtual float get_drug_gametocyte02() = 0;
        virtual float get_drug_gametocyte34() = 0;
        virtual float get_drug_gametocyteM() = 0;
        virtual ~IMalariaDrugEffects() { }
    };

    struct IMalariaDrugEffectsApply : public ISupports
    {
        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate ) = 0;
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate ) = 0;
        virtual void ApplyDrugIRBCKillRateEffect( float rate ) = 0;
        virtual void ApplyDrugHepatocyteEffect( float rate ) = 0;
        virtual void ApplyDrugGametocyte02Effect( float rate ) = 0;
        virtual void ApplyDrugGametocyte34Effect( float rate ) = 0;
        virtual void ApplyDrugGametocyteMEffect( float rate ) = 0;
    };

    struct IDrug;
    class MalariaInterventionsContainer :
        public VectorInterventionsContainer,
        public IMalariaDrugEffects, // Getters
        public IMalariaDrugEffectsApply
    {
    public:
        // TODO - WHY IS THIS NECESSARY? Making compiler happy (but not me). Make go away soon.
        virtual int32_t AddRef() override;
        virtual int32_t Release() override;

        MalariaInterventionsContainer();
        virtual ~MalariaInterventionsContainer();

        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;

        // IDistributableIntervention
        virtual bool GiveIntervention( IDistributableIntervention * pIV ) override;

        // IMalariaDrugEffectsApply
        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate ) override;
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate ) override;
        virtual void ApplyDrugIRBCKillRateEffect( float rate ) override;
        virtual void ApplyDrugHepatocyteEffect( float rate ) override;
        virtual void ApplyDrugGametocyte02Effect( float rate ) override;
        virtual void ApplyDrugGametocyte34Effect( float rate ) override;
        virtual void ApplyDrugGametocyteMEffect( float rate ) override;

        //IMalariaDrugEffects(Get): TODO move impl to cpp.
        virtual float get_drug_IRBC_killrate() override;
        virtual float get_drug_hepatocyte() override;
        virtual float get_drug_gametocyte02() override;
        virtual float get_drug_gametocyte34() override;
        virtual float get_drug_gametocyteM() override;

        virtual void Update(float dt) override; // hook to update interventions if they need it

    protected:
        float drug_IRBC_killrate;
        float drug_hepatocyte;
        float drug_gametocyte02;
        float drug_gametocyte34;
        float drug_gametocyteM;

        IMalariaHumanContext* individual;

        bool GiveDrug(IDrug* drug); //do some special stuff for drugs.

        DECLARE_SERIALIZABLE(MalariaInterventionsContainer);
    };
}
