/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "ReportMalaria.h" // for base class
#include "MalariaContexts.h" // for base class

static const string _parasite_prevalence_label( "Parasite Prevalence" );
static const string _mean_parasitemia_label( "Mean Parasitemia" );
static const string _new_diagnostic_prevalence_label( "New Diagnostic Prevalence" );
static const string _fever_prevalence_label( "Fever Prevalence" );
static const string _new_clinical_cases_label( "New Clinical Cases" );
static const string _new_severe_cases_label( "New Severe Cases" );
static const string _statistical_population_label( "Statistical Population" );

namespace Kernel {

    ReportMalaria::ReportMalaria()
    {}

    void
    ReportMalaria::populateSummaryDataUnitsMap(
        std::map<std::string, std::string> &units_map
    )
    {
        ReportVector::populateSummaryDataUnitsMap(units_map);
        
        // Additional malaria channels
        units_map[_parasite_prevalence_label]       = "Infected %";
        units_map[_mean_parasitemia_label]          = "Geo. mean parasites/microliter";
        units_map[_new_diagnostic_prevalence_label] = "Infected %";
        units_map[_fever_prevalence_label]          = "Infected %";
        units_map[_new_clinical_cases_label]        = "";
        units_map[_new_severe_cases_label]          = "";
    }

    void
    ReportMalaria::postProcessAccumulatedData()
    {
        ReportVector::postProcessAccumulatedData();

        // make sure to normalize Mean Parasitemia BEFORE Parasite Prevalence, then it is exponentiated
        normalizeChannel(_mean_parasitemia_label, _parasite_prevalence_label);
        channelDataMap.ExponentialValues( _mean_parasitemia_label );

        // now normalize rest of channels
        normalizeChannel(_parasite_prevalence_label, _statistical_population_label);
        normalizeChannel(_new_diagnostic_prevalence_label, _statistical_population_label);
        normalizeChannel(_fever_prevalence_label, _statistical_population_label);
    }


    void
    ReportMalaria::LogNodeData(
        INodeContext * pNC
    )
    {
        ReportVector::LogNodeData( pNC );

        const INodeMalaria* pMalariaNode = nullptr;
        if( pNC->QueryInterface( GET_IID(INodeMalaria), (void**)&pMalariaNode ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeMalaria", "INodeContext" );
        }
        Accumulate(_parasite_prevalence_label,       pMalariaNode->GetParasitePositive());
        Accumulate(_mean_parasitemia_label,          pMalariaNode->GetLogParasites());
        Accumulate(_new_diagnostic_prevalence_label, pMalariaNode->GetNewDiagnosticPositive());
        Accumulate(_fever_prevalence_label,          pMalariaNode->GetFeverPositive());
        Accumulate(_new_clinical_cases_label,        pMalariaNode->GetNewClinicalCases());
        Accumulate(_new_severe_cases_label,          pMalariaNode->GetNewSevereCases());
    }
}
