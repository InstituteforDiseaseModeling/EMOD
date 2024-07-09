
#pragma once

#include <list>
#include <map>
#include <vector>
#include <string>
#include <fstream>

#include "SpatialReport.h"

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
