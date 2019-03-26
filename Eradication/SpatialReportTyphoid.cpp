/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TYPHOID

#include <functional>
#include <numeric>
#include <map>
#include "BoostLibWrapper.h"

#include "SpatialReportTyphoid.h"
//#include "NodeTyphoid.h"
#include "Sugar.h"
#include "Environment.h"
#include "Exceptions.h"
#include "Individual.h"
#include "SimulationConfig.h"
#include "ProgVersion.h"

using namespace std;

static const char * _module = "SpatialReportTyphoid";

namespace Kernel {

GET_SCHEMA_STATIC_WRAPPER_IMPL(SpatialReportTyphoid,SpatialReportTyphoid)

/////////////////////////
// Initialization methods
/////////////////////////
IReport*
SpatialReportTyphoid::CreateReport()
{
    return new SpatialReportTyphoid();
}

SpatialReportTyphoid::SpatialReportTyphoid()
: SpatialReportEnvironmental()
{
}

void SpatialReportTyphoid::populateChannelInfos(tChanInfoMap &channel_infos)
{
    SpatialReportEnvironmental::populateChannelInfos(channel_infos);
}


void SpatialReportTyphoid::LogIndividualData( IIndividualHuman * individual )
{
    SpatialReportEnvironmental::LogIndividualData(individual);

    float monte_carlo_weight = (float)individual->GetMonteCarloWeight();

    NewInfectionState::_enum nis = individual->GetNewInfectionState();
}

void
SpatialReportTyphoid::LogNodeData(
    INodeContext * pNC
)
{
    SpatialReport::LogNodeData(pNC);

    //int nodeid = pNC->GetExternalID();
}

void
SpatialReportTyphoid::postProcessAccumulatedData()
{
    SpatialReport::postProcessAccumulatedData();

    // pass through normalization
    // order matters, since we're changing channels in place (not like old way)
}

}

#endif // ENABLE_TYPHOID
