/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include "HIVSimpleDiagnostic.h"
#include "EventTrigger.h"

namespace Kernel
{
    class HIVRandomChoice : public HIVSimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVRandomChoice, IDistributableIntervention)

    public: 
	HIVRandomChoice();
        virtual bool Configure( const Configuration * inputJson ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // HIVSimpleDiagnostic
        virtual bool positiveTestResult() override;
        virtual void positiveTestDistribute() override;
        
    protected:
        virtual void ProcessChoices(std::vector<std::string> &names, std::vector<float> &values);

        std::vector<EventTrigger> event_names;
        std::vector<float> event_probabilities;

        DECLARE_SERIALIZABLE(HIVRandomChoice);
    };
}
