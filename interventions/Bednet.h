/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "IWaningEffect.h"
#include "EventTrigger.h"

namespace Kernel
{
    ENUM_DEFINE(BednetType,
        ENUM_VALUE_SPEC(Barrier     , 1)
        ENUM_VALUE_SPEC(ITN         , 2)
        ENUM_VALUE_SPEC(LLIN        , 3)
        ENUM_VALUE_SPEC(Retreatment , 4))

    struct IBednetConsumer;

    /* Keep around as an identity solution??? */
    struct IBednet : public ISupports
    {
    };

    class SimpleBednet : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleBednet, IDistributableIntervention)

    public:
        SimpleBednet();
        SimpleBednet( const SimpleBednet& );
        virtual ~SimpleBednet();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

    protected:

        WaningConfig   killing_config;
        IWaningEffect* killing_effect;
        WaningConfig   blocking_config;
        IWaningEffect* blocking_effect;
        BednetType::Enum bednet_type;
        EventTrigger on_distributed_event ;
        IBednetConsumer *ibc;

        DECLARE_SERIALIZABLE(SimpleBednet);
    };
}
