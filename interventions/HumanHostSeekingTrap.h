/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
        HumanHostSeekingTrap();
        HumanHostSeekingTrap( const HumanHostSeekingTrap& );
        virtual ~HumanHostSeekingTrap();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

    protected:
        IWaningEffect* killing_effect;
        IWaningEffect* attract_effect;
        IVectorInterventionEffectsSetter *ivies;

        DECLARE_SERIALIZABLE(HumanHostSeekingTrap);
    };
}
