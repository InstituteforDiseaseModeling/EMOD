/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Interventions.h"
#include "Contexts.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "IWaningEffect.h"

namespace Kernel
{
    ENUM_DEFINE(SimpleVaccineType,
        ENUM_VALUE_SPEC(Generic              , 1)
        ENUM_VALUE_SPEC(TransmissionBlocking , 2)
        ENUM_VALUE_SPEC(AcquisitionBlocking  , 3)
        ENUM_VALUE_SPEC(MortalityBlocking    , 4))

    struct IVaccineConsumer;
    struct ICampaignCostObserver;

    struct IVaccine : public ISupports
    {
        virtual void  ApplyVaccineTake()                = 0;
        virtual ~IVaccine() { } // needed for cleanup via interface pointer
    };

    class SimpleVaccine : public BaseIntervention, public IVaccine
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleVaccine, IDistributableIntervention)

    public:
        SimpleVaccine();
        SimpleVaccine( const SimpleVaccine& );
        virtual ~SimpleVaccine();
        virtual int AddRef() override { return BaseIntervention::AddRef(); }
        virtual int Release() override { return BaseIntervention::Release(); }
        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // IVaccine
        virtual void  ApplyVaccineTake() override;

    protected:
        // context for this intervention--does not need to be reset upon migration, it is just for GiveVaccine()
        IIndividualHumanContext *parent;

        int   vaccine_type;
        float vaccine_take;
        float current_reducedacquire;
        float current_reducedtransmit;
        float current_reducedmortality;
        WaningConfig   waning_config;
        IWaningEffect* waning_effect;
        IVaccineConsumer * ivc; // interventions container

        DECLARE_SERIALIZABLE(SimpleVaccine);
    };
}
