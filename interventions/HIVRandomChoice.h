/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "HIVSimpleDiagnostic.h"

namespace Kernel
{
    class Event2ProbabilityMapType : public JsonConfigurable, public JsonConfigurable::tStringFloatMapConfigType
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            Event2ProbabilityMapType() {}
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            virtual json::QuickBuilder GetSchema();

            static void serialize(IArchive& ar, Event2ProbabilityMapType& obj);
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
        Event2ProbabilityMapType event2ProbabilityMap;

        DECLARE_SERIALIZABLE(HIVRandomChoice);
    };
}
