/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <vector>

#include "Diagnostics.h"

namespace Kernel
{
    class CD4Thresholds : public JsonConfigurable
    {
        friend class ::boost::serialization::access;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        public:
            CD4Thresholds() {}
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            virtual json::QuickBuilder GetSchema();
            std::vector<std::pair< NaturalNumber, NaturalNumber > > thresholds;
            std::vector< std::string > thresh_events;
    };

    class CD4Diagnostic : public SimpleDiagnostic 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, CD4Diagnostic, IDistributableIntervention)

    public: 
        CD4Diagnostic();
        CD4Diagnostic( const CD4Diagnostic& );
        virtual bool Configure( const Configuration* pConfig );
        virtual ~CD4Diagnostic();

    protected:
        virtual bool positiveTestResult();
        CD4Thresholds cd4_thresholds;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        // Serialization
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, CD4Diagnostic &obj, const unsigned int v);
#endif
    };
}


