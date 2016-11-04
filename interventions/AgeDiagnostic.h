/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <vector>

#include "Diagnostics.h"

namespace Kernel
{
    class AgeThresholds : public JsonConfigurable, public IComplexJsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        public:
            AgeThresholds() {}
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
            virtual json::QuickBuilder GetSchema() override;
            std::vector<std::pair< NaturalNumber, NaturalNumber > > thresholds;
            std::vector< std::string > thresh_events;

            static void serialize(IArchive& ar, AgeThresholds& obj);
    };

    class AgeDiagnostic : public SimpleDiagnostic 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, AgeDiagnostic, IDistributableIntervention)

    public: 
        AgeDiagnostic();
        AgeDiagnostic( const AgeDiagnostic& );
        virtual bool Configure( const Configuration* pConfig );
        virtual ~AgeDiagnostic();

    protected:
        virtual bool positiveTestResult();
        AgeThresholds age_thresholds;

        DECLARE_SERIALIZABLE(AgeDiagnostic);
    };
}
