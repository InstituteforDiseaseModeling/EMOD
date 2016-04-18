/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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

class SpatialReportPy : public SpatialReport
{
    GET_SCHEMA_STATIC_WRAPPER(SpatialReportPy)

public:
    static IReport* CreateReport();
    virtual ~SpatialReportPy() { }

    virtual void LogIndividualData( IIndividualHuman * individual );
    virtual void LogNodeData( INodeContext * pNC );

protected:
    SpatialReportPy();

    virtual void postProcessAccumulatedData();

    virtual void populateChannelInfos(tChanInfoMap &channel_infos);

    // counters for LogIndividualData stuff 

private:
#if USE_BOOST_SERIALIZATION
    friend class ::boost::serialization::access;
    template<class Archive>
    friend void serialize(Archive &ar, SpatialReportPy& report, const unsigned int v);
#endif
};
}
