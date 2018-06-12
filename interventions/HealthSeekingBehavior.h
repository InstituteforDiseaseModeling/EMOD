/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "Interventions.h"
#include "Contexts.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "NodeEventContext.h"    // for INodeEventContext (ICampaignCostObserver)
#include "EventTrigger.h"

namespace Kernel
{

    class IDMAPI SimpleHealthSeekingBehavior : public BaseIntervention //, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleHealthSeekingBehavior, IDistributableIntervention)

    public:
        SimpleHealthSeekingBehavior();
        virtual ~SimpleHealthSeekingBehavior() {  }

        // We inherit AddRef/Release abstractly through IHealthSeekBehavior,
        // even though BaseIntervention has a non-abstract version.
        virtual int32_t AddRef() override { return BaseIntervention::AddRef(); }
        virtual int32_t Release() override { return BaseIntervention::Release(); }

        virtual bool Configure( const Configuration* config ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

        //virtual void Expire();

    protected:

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        ICampaignCostObserver * m_pCCO;
        float probability_of_seeking;
        EventOrConfig::Enum use_event_or_config;
        IndividualInterventionConfig actual_intervention_config;
        EventTrigger actual_intervention_event;
        bool single_use;

        DECLARE_SERIALIZABLE(SimpleHealthSeekingBehavior);
#pragma warning( pop )
    };
}
