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

#include "SpatialReport.h"
#include "BoostLibWrapper.h"

namespace Kernel {

class SpatialReportVector : public SpatialReport
{
    GET_SCHEMA_STATIC_WRAPPER(SpatialReportVector)

public:
    static IReport* CreateReport();
    virtual ~SpatialReportVector() { }

    virtual void Initialize( unsigned int nrmSize );
    virtual void LogNodeData( Kernel::INodeContext * pNode );

protected:
    SpatialReportVector();

    virtual void populateChannelInfos(tChanInfoMap &channel_infos);
    virtual void postProcessAccumulatedData();

    ChannelInfo adult_vectors_info;
    ChannelInfo infectious_vectors_info;
    ChannelInfo daily_eir_info;
    ChannelInfo daily_hbr_info;
};
}
