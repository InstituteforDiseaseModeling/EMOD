/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "HIVSimpleDiagnostic.h"

namespace Kernel
{
    class Event2ProbabilityMapType : public JsonConfigurable, public JsonConfigurable::tStringFloatMapConfigType
    {
        friend class ::boost::serialization::access;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            Event2ProbabilityMapType() {}
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            virtual json::QuickBuilder GetSchema();
    };

    class HIVRandomChoice : public HIVSimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVRandomChoice, IDistributableIntervention)

    public: 
        HIVRandomChoice();
        HIVRandomChoice( const HIVRandomChoice& );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        // HIVSimpleDiagnostic
        virtual bool positiveTestResult();
        virtual void positiveTestDistribute();

    protected:

        Event2ProbabilityMapType event2ProbabilityMap;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, HIVRandomChoice &obj, const unsigned int v);
#endif
    };
}
