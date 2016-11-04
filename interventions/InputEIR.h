/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "EventCoordinator.h"
#include "Configure.h"
#include "VectorEnums.h"
#include "VectorDefs.h"
#include "Common.h"

namespace Kernel
{
    class InputEIRConfig : public JsonConfigurable, 
                           public IComplexJsonConfigurable, 
                           public std::vector<float>
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        public:
            InputEIRConfig() : std::vector<float>(MONTHSPERYEAR) {}
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            virtual json::QuickBuilder GetSchema();
    };

    class InputEIR : public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(Outbreak)
        DECLARE_FACTORY_REGISTERED(InterventionFactory, InputEIR, INodeDistributableIntervention)

    public:
        InputEIR();
        InputEIR( const InputEIR& master );
        virtual ~InputEIR() { }

        // INodeDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(INodeEventContext *context);
        virtual void Update(float dt);

    protected:
        INodeEventContext* parent;
        AgeDependentBitingRisk::Enum age_dependence;
        InputEIRConfig monthly_EIR; // 12 values of EIR by month
        float today;
        float daily_EIR;
        tAgeBitingFunction risk_function;
    };
}
