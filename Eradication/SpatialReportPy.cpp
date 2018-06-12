/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#pragma warning(disable:4996)

#ifdef ENABLE_PYTHON

#include <functional>
#include <numeric>
#include <map>
#include "BoostLibWrapper.h"

#include "SpatialReportPy.h"
#include "NodePy.h"
#include "Sugar.h"
#include "Environment.h"
#include "Exceptions.h"
#include "Individual.h"
#include "SimulationConfig.h"
#include "ProgVersion.h"

using namespace std;

SETUP_LOGGING( "SpatialReportPy" )

namespace Kernel {

GET_SCHEMA_STATIC_WRAPPER_IMPL(SpatialReportPy,SpatialReportPy)

/////////////////////////
// Initialization methods
/////////////////////////
IReport*
SpatialReportPy::CreateReport()
{
    return new SpatialReportPy();
}

SpatialReportPy::SpatialReportPy()
: SpatialReport()
{
}

void SpatialReportPy::populateChannelInfos(tChanInfoMap &channel_infos)
{
    SpatialReport::populateChannelInfos(channel_infos);
}


void SpatialReportPy::LogIndividualData( IIndividualHuman * individual )
{
    SpatialReport::LogIndividualData(individual);

    float monte_carlo_weight = (float)individual->GetMonteCarloWeight();

    NewInfectionState::_enum nis = individual->GetNewInfectionState();
}

void
SpatialReportPy::LogNodeData(
    INodeContext * pNC
)
{
    SpatialReport::LogNodeData(pNC);

    int nodeid = pNC->GetExternalID();

    const Kernel::INodePy * pPyNode = NULL;
    if( pNC->QueryInterface( GET_IID( Kernel::INodePy), (void**) &pPyNode ) != Kernel::s_OK )
    {
        throw Kernel::QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodePy", "INodeContext" );
    }
}

void
SpatialReportPy::postProcessAccumulatedData()
{
    SpatialReport::postProcessAccumulatedData();

    // pass through normalization
    // order matters, since we're changing channels in place (not like old way)
}

}

#endif // ENABLE_PYTHON
