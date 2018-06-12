/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <list>
#include <map>
#include <vector>
#include <string>
#include <fstream>

#include "SpatialReportVector.h"
#include "BoostLibWrapper.h"

namespace Kernel {

class SpatialReportMalaria : public SpatialReportVector
{
    GET_SCHEMA_STATIC_WRAPPER(SpatialReportMalaria)

public:
    static IReport* CreateReport();
    virtual ~SpatialReportMalaria() { }

    virtual void Initialize( unsigned int nrmSize );

    virtual void LogNodeData( Kernel::INodeContext * pNC );

protected:
    SpatialReportMalaria();

    virtual void postProcessAccumulatedData();

    virtual void populateChannelInfos(tChanInfoMap &channel_infos);

    ChannelInfo parasite_prevalence_info;
    ChannelInfo mean_parasitemia_info;
    ChannelInfo new_diagnostic_prevalence_info;
    ChannelInfo fever_prevalence_info;
    ChannelInfo new_clinical_cases_info;
    ChannelInfo new_severe_cases_info;
};
}
