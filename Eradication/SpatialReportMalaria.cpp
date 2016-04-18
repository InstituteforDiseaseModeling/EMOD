/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <functional>
#include <map>

#include "SpatialReportMalaria.h"
#include "MalariaContexts.h"
#include "Sugar.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IIndividualHuman.h"
#include "ProgVersion.h"

using namespace std;

static const char * _module = "SpatialReportMalaria";

namespace Kernel {

GET_SCHEMA_STATIC_WRAPPER_IMPL(SpatialReportMalaria,SpatialReportMalaria)

/////////////////////////
// Initialization methods
/////////////////////////
IReport*
SpatialReportMalaria::CreateReport()
{
    return new SpatialReportMalaria();
}

SpatialReportMalaria::SpatialReportMalaria()
: SpatialReportVector()
, parasite_prevalence_info(         "Parasite_Prevalence",          "infected fraction")
, mean_parasitemia_info(            "Mean_Parasitemia",             "geo. mean parasites/microliter")
, new_diagnostic_prevalence_info(   "New_Diagnostic_Prevalence",    "positive fraction")
, fever_prevalence_info(            "Fever_Prevalence",             "fraction")
, new_clinical_cases_info(          "New_Clinical_Cases",           "")
, new_severe_cases_info(            "New_Severe_Cases",             "")
{
}

void SpatialReportMalaria::Initialize( unsigned int nrmSize )
{
    SpatialReportVector::Initialize( nrmSize );

    if( mean_parasitemia_info.enabled && !parasite_prevalence_info.enabled )
    {
        LOG_WARN("Mean_Parasitemia requires that Parasite_Prevalence be enabled.  Enabling Parasite_Prevalence.");
        parasite_prevalence_info.enabled = true ;
        channelDataMap.IncreaseChannelLength( parasite_prevalence_info.name, _nrmSize );
    }
}

void SpatialReportMalaria::populateChannelInfos(tChanInfoMap &channel_infos)
{
    SpatialReportVector::populateChannelInfos(channel_infos);

    channel_infos[ parasite_prevalence_info.name ] = &parasite_prevalence_info;
    channel_infos[ mean_parasitemia_info.name ] = &mean_parasitemia_info;
    channel_infos[ new_diagnostic_prevalence_info.name ] = &new_diagnostic_prevalence_info;
    channel_infos[ fever_prevalence_info.name ] = &fever_prevalence_info;
    channel_infos[ new_clinical_cases_info.name ] = &new_clinical_cases_info;
    channel_infos[ new_severe_cases_info.name ] = &new_severe_cases_info;
}


void
SpatialReportMalaria::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    SpatialReportVector::LogNodeData(pNC);

    auto nodeid = pNC->GetExternalID();

    const INodeMalaria* pMalariaNode = nullptr;
    if( pNC->QueryInterface( GET_IID(INodeMalaria), (void**)&pMalariaNode ) != s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeMalaria", "INodeContext" );
    }

    if(parasite_prevalence_info.enabled)
        Accumulate(parasite_prevalence_info.name, nodeid, pMalariaNode->GetParasitePositive());

    if(mean_parasitemia_info.enabled)
        Accumulate(mean_parasitemia_info.name, nodeid, pMalariaNode->GetLogParasites());

    if(new_diagnostic_prevalence_info.enabled)
        Accumulate(new_diagnostic_prevalence_info.name, nodeid, pMalariaNode->GetNewDiagnosticPositive());

    if(fever_prevalence_info.enabled)
        Accumulate(fever_prevalence_info.name, nodeid, pMalariaNode->GetFeverPositive());

    if(new_clinical_cases_info.enabled)
        Accumulate(new_clinical_cases_info.name, nodeid, pMalariaNode->GetNewClinicalCases());

    if(new_severe_cases_info.enabled)
        Accumulate(new_severe_cases_info.name, nodeid, pMalariaNode->GetNewSevereCases());
}

void
SpatialReportMalaria::postProcessAccumulatedData()
{
    SpatialReportVector::postProcessAccumulatedData();

    // make sure to normalize Mean Parasitemia BEFORE Parasite Prevalence, then it is exponentiated
    if( mean_parasitemia_info.enabled && parasite_prevalence_info.enabled )
    {
        normalizeChannel(mean_parasitemia_info.name, parasite_prevalence_info.name);

        // Only need to transform mean log-parasitemia to geometric-mean parasitemia if that channel is enabled.  
        if( channelDataMap.HasChannel( mean_parasitemia_info.name ) )
        {
            channelDataMap.ExponentialValues( mean_parasitemia_info.name );
        }
    }
    else if( mean_parasitemia_info.enabled && !parasite_prevalence_info.enabled )
    {
        throw GeneralConfigurationException(  __FILE__, __LINE__, __FUNCTION__, "If 'Mean_Parasitemia' is enabled, then 'Parasite_Prevalence' must be enabled.");
    }

    // now normalize rest of channels
    if( parasite_prevalence_info.enabled )
        normalizeChannel(parasite_prevalence_info.name, population_info.name);

    if( new_diagnostic_prevalence_info.enabled )
        normalizeChannel(new_diagnostic_prevalence_info.name, population_info.name);

    if( fever_prevalence_info.enabled )
        normalizeChannel(fever_prevalence_info.name, population_info.name);
}


#if 0
template<class Archive>
void serialize(Archive &ar, SpatialReportMalaria& report, const unsigned int v)
{
    ar & report.timesteps_reduced;
    ar & report.channelDataMap;
    ar & report._nrmSize;
}
#endif

}
