/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Diagnostics.h"

namespace Kernel
{
    class ImmunityBloodTest : public SimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ImmunityBloodTest, IDistributableIntervention)

        float threshold_acquisitionImmunity;
        EventTrigger negative_diagnosis_event;

    public:
        ImmunityBloodTest();
        ImmunityBloodTest(const ImmunityBloodTest&);
        virtual ~ImmunityBloodTest();
        virtual bool Configure(const Configuration* pConfig) override;
        virtual EventOrConfig::Enum getEventOrConfig( const Configuration * inputJson ) override;
        virtual void CheckConfigTriggers( const Configuration * inputJson ) override;
        virtual bool positiveTestResult() override;
        virtual void onNegativeTestResult() override;

        DECLARE_SERIALIZABLE(ImmunityBloodTest);
    };
}
