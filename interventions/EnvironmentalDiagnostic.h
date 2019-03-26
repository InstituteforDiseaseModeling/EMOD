/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configuration.h"
#include "Configure.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "INodeContext.h"
#include "EventTriggerNode.h"

namespace Kernel
{
    class EnvironmentalDiagnostic: public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, EnvironmentalDiagnostic, INodeDistributableIntervention)

    public:
        EnvironmentalDiagnostic();
        EnvironmentalDiagnostic(const EnvironmentalDiagnostic&);
        virtual ~EnvironmentalDiagnostic() {};
        virtual bool Configure(const Configuration* pConfig);
        virtual void Update(float dt) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC = nullptr ) override;

    protected:
        virtual void performTest();
        float sample_threshold;
        float base_specificity;
        float base_sensitivity;
        EventTriggerNode negative_diagnosis_event, positive_diagnosis_event;
        IPKeyValue environment_ip_key_value;
    };
}
