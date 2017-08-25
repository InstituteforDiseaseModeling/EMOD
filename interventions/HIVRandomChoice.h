/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include "HIVSimpleDiagnostic.h"
#include "EventTrigger.h"

namespace Kernel
{
    class Event2ProbabilityType : public JsonConfigurable, 
                                  public IComplexJsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            Event2ProbabilityType() {}
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
            virtual json::QuickBuilder GetSchema() override;
            virtual bool  HasValidDefault() const override { return false; }

            std::vector<std::pair<EventTrigger,float>> event_list;

            static void serialize(IArchive& ar, Event2ProbabilityType& obj);
    };

    class HIVRandomChoice : public HIVSimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVRandomChoice, IDistributableIntervention)

    public: 
        HIVRandomChoice();
        HIVRandomChoice( const HIVRandomChoice& );

        virtual bool Configure( const Configuration * inputJson ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // HIVSimpleDiagnostic
        virtual bool positiveTestResult() override;
        virtual void positiveTestDistribute() override;

    protected:
        Event2ProbabilityType event2Probability;

        DECLARE_SERIALIZABLE(HIVRandomChoice);
    };
}
