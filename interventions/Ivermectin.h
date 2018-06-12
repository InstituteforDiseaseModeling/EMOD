/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>

#include "Interventions.h"
#include "InterventionFactory.h"
#include "Configuration.h"
#include "IWaningEffect.h"
#include "Configure.h"

namespace Kernel
{
    struct IVectorInterventionEffectsSetter;

    class Ivermectin : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, Ivermectin, IDistributableIntervention)

    public:
        Ivermectin();
        Ivermectin( const Ivermectin& );
        virtual ~Ivermectin();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

    protected:
        IWaningEffect* killing_effect;
        IVectorInterventionEffectsSetter *ivies; // aka individual vector interventions container

        DECLARE_SERIALIZABLE(Ivermectin);
    };
}
