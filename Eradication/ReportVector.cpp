
#include "stdafx.h"
#include "ReportVector.h" // for base class
#include "Log.h" // for base class
#include "VectorContexts.h"
#include "INodeContext.h"

SETUP_LOGGING( "VectorReporter" )

namespace Kernel
{
ReportVector::ReportVector()
    : Report()
    , adult_vectors_id()
    , infectious_vectors_id()
    , daily_EIR_id()
    , daily_HBR_id()
{
    adult_vectors_id      = AddChannel( "Adult Vectors"         );
    infectious_vectors_id = AddChannel( "Infectious Vectors"    );
    daily_EIR_id          = AddChannel( "Daily EIR"             );
    daily_HBR_id          = AddChannel( "Daily Bites per Human" );
}

void
ReportVector::populateSummaryDataUnitsMap(
    std::map<std::string, std::string> &units_map
)
{
    LOG_DEBUG( "populateSummaryDataUnitsMap\n" );
    Report::populateSummaryDataUnitsMap(units_map);

    // Additional vector channels
    units_map[ adult_vectors_id.GetName()      ] = "Vectors";
    units_map[ infectious_vectors_id.GetName() ] = "Infectious %";
    units_map[ daily_EIR_id.GetName()          ] = "Infectious Bites/Day";
    units_map[ daily_HBR_id.GetName()          ] = "Bites/Day";
}

void
ReportVector::postProcessAccumulatedData()
{
    LOG_DEBUG( "postProcessAccumulatedData\n" );
    Report::postProcessAccumulatedData();

    // careful with order, we normalize 'in place', so channels used as
    // normalization 'bases' should be normalized last.
    normalizeChannel( infectious_vectors_id.GetName(), adult_vectors_id.GetName() );
    normalizeChannel( adult_vectors_id.GetName(),      (float)_nrmSize            );
    normalizeChannel( daily_EIR_id.GetName(),          stat_pop_id.GetName()      );
    normalizeChannel( daily_HBR_id.GetName(),          stat_pop_id.GetName()      );
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
        adult_vectors += float( vectorpopulation->getCount( VectorStateEnum::STATE_ADULT ) );
        adult_vectors += float( vectorpopulation->getCount( VectorStateEnum::STATE_INFECTED ) );
        adult_vectors += float( vectorpopulation->getCount( VectorStateEnum::STATE_INFECTIOUS ) );

        infectious_vectors += (float)(vectorpopulation->getCount( VectorStateEnum::STATE_INFECTIOUS ));

        daily_eir          += vectorpopulation->GetEIRByPool(Kernel::VectorPoolIdEnum::BOTH_VECTOR_POOLS);
        daily_hbr          += vectorpopulation->GetHBRByPool(Kernel::VectorPoolIdEnum::BOTH_VECTOR_POOLS);
    }

    float node_pop = pNC->GetStatPop();

    Accumulate( adult_vectors_id,      adult_vectors          );
    Accumulate( infectious_vectors_id, infectious_vectors     );
    Accumulate( daily_EIR_id,          (daily_eir * node_pop) );
    Accumulate( daily_HBR_id,          (daily_hbr * node_pop) );
}

}
