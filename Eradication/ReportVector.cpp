/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "ReportVector.h" // for base class
#include "Log.h" // for base class
#include "VectorContexts.h"
#include "VectorPopulation.h"

SETUP_LOGGING( "VectorReporter" )

static const string _adult_vectors_label( "Adult Vectors" );
static const string _infectious_vectors_label( "Infectious Vectors" );
static const string _daily_eir_label( "Daily EIR" );
static const string _daily_bites_per_human_label( "Daily Bites per Human" );

namespace Kernel {

ReportVector::ReportVector()
{}

void
ReportVector::populateSummaryDataUnitsMap(
    std::map<std::string, std::string> &units_map
)
{
    LOG_DEBUG( "populateSummaryDataUnitsMap\n" );
    Report::populateSummaryDataUnitsMap(units_map);

    // Additional vector channels
    units_map[_adult_vectors_label]         = "Vectors";
    units_map[_infectious_vectors_label]    = "Infectious %";
    units_map[_daily_eir_label]             = "Infectious Bites/Day";
    units_map[_daily_bites_per_human_label] = "Bites/Day";
}

void
ReportVector::postProcessAccumulatedData()
{
    LOG_DEBUG( "postProcessAccumulatedData\n" );
    Report::postProcessAccumulatedData();

    // careful with order, we normalize 'in place', so channels used as
    // normalization 'bases' should be normalized last.
    normalizeChannel(_infectious_vectors_label,    "Adult Vectors");
    normalizeChannel(_adult_vectors_label,         (float)_nrmSize);
    normalizeChannel(_daily_eir_label,             _nrmSize);
    normalizeChannel(_daily_bites_per_human_label, (float)_nrmSize);
}

void
ReportVector::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    Report::LogNodeData( pNC );

    float adult_vectors      = 0;
    float infectious_vectors = 0;
    float daily_eir          = 0;
    float daily_hbr          = 0;

    INodeVector* pNV = nullptr;
    if( pNC->QueryInterface( GET_IID( INodeVector ), (void**) & pNV ) != s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext" );
    }

    const VectorPopulationReportingList_t& vectorPopulations = pNV->GetVectorPopulationReporting();

    for (const auto vectorpopulation : vectorPopulations)
    {
        adult_vectors      += (float)( vectorpopulation->getAdultCount() + vectorpopulation->getInfectedCount() + vectorpopulation->getInfectiousCount() );
        infectious_vectors += (float)( vectorpopulation->getInfectiousCount() );

        daily_eir          += vectorpopulation->GetEIRByPool(Kernel::VectorPoolIdEnum::BOTH_VECTOR_POOLS);
        daily_hbr          += vectorpopulation->GetHBRByPool(Kernel::VectorPoolIdEnum::BOTH_VECTOR_POOLS);
    }

    Accumulate(_adult_vectors_label,         adult_vectors);
    Accumulate(_infectious_vectors_label,    infectious_vectors);
    Accumulate(_daily_eir_label,             daily_eir);
    Accumulate(_daily_bites_per_human_label, daily_hbr);
}

}
