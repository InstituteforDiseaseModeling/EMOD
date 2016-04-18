/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"


namespace Kernel
{
    struct IHealthSeekingBehaviorUpdateEffectsApply;

    class HealthSeekingBehaviorUpdate : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HealthSeekingBehaviorUpdate, IDistributableIntervention)

    public: 
        bool Configure( const Configuration * config );
        HealthSeekingBehaviorUpdate();

        // factory method
        virtual ~HealthSeekingBehaviorUpdate() { };

        // We inherit AddRef/Release abstractly through IHealthSeekBehavior,
        // even though BaseIntervention has a non-abstract version.
        virtual int32_t AddRef() { return BaseIntervention::AddRef(); }
        virtual int32_t Release() { return BaseIntervention::Release(); }

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

    protected:
        float new_probability_of_seeking;
        IHealthSeekingBehaviorUpdateEffectsApply *ihsbuea;
    };
}

