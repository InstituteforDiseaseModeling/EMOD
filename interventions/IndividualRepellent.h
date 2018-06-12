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
#include "InterventionFactory.h"
#include "Configuration.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "IWaningEffect.h"

namespace Kernel
{
    struct IIndividualRepellentConsumer;

    class SimpleIndividualRepellent : public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleIndividualRepellent, IDistributableIntervention)

    public:
        virtual bool Configure( const Configuration * config ) override;

        SimpleIndividualRepellent();
        SimpleIndividualRepellent( const SimpleIndividualRepellent& );
        virtual ~SimpleIndividualRepellent();

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt);

    protected:
        IWaningEffect* blocking_effect;
        IIndividualRepellentConsumer *ihmc; // aka individual or individual vector interventions container

        DECLARE_SERIALIZABLE(SimpleIndividualRepellent);
    };
}
