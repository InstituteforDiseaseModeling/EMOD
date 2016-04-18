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
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "IWaningEffect.h"

namespace Kernel
{
    struct IVectorInterventionEffectsSetter;

    /* Keep around as an identity solution??? */
    struct IHumanHostSeekingTrap : public ISupports
    {
    };

    class HumanHostSeekingTrap : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HumanHostSeekingTrap, IDistributableIntervention)

    public:
        bool Configure( const Configuration * config );
        HumanHostSeekingTrap();
        HumanHostSeekingTrap( const HumanHostSeekingTrap& );
        virtual ~HumanHostSeekingTrap();

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

    protected:
        float current_attractrate;
        float current_killingrate;
        WaningConfig   killing_config;
        IWaningEffect* killing_effect;
        WaningConfig   attract_config;
        IWaningEffect* attract_effect;
        IVectorInterventionEffectsSetter *ivies;

        DECLARE_SERIALIZABLE(HumanHostSeekingTrap);
    };
}
