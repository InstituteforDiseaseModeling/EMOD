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
        bool Configure( const Configuration * config );

        SimpleIndividualRepellent();
        SimpleIndividualRepellent( const SimpleIndividualRepellent& );
        virtual ~SimpleIndividualRepellent();

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

        // IIndividualRepellent
        virtual float GetBlockingRate() const { return current_blockingrate; }
        virtual float GetKillingRate() const { return current_killingrate; }

    protected:
        float current_killingrate;
        float current_blockingrate;
//        WaningConfig   killing_config;
//        IWaningEffect* killing_effect;
        WaningConfig   blocking_config;
        IWaningEffect* blocking_effect;
        IIndividualRepellentConsumer *ihmc; // aka individual or individual vector interventions container

        DECLARE_SERIALIZABLE(SimpleIndividualRepellent);
    };
}
