/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#pragma once

#include <list>
#include <map>
#include <vector>
#include <string>
#include <fstream>

#include "SpatialReport.h"
#include "BoostLibWrapper.h"

namespace Kernel {

    typedef SpatialReport SpatialReportEnvironmental;
    class SpatialReportTyphoid : public SpatialReportEnvironmental
    {
        GET_SCHEMA_STATIC_WRAPPER(SpatialReportTyphoid)

        public:
            static IReport* CreateReport();
            virtual ~SpatialReportTyphoid() { }

            virtual void LogIndividualData( IIndividualHuman * individual );
            virtual void LogNodeData( INodeContext * pNC );

        protected:
            SpatialReportTyphoid();

            virtual void postProcessAccumulatedData();

            virtual void populateChannelInfos(tChanInfoMap &channel_infos);

            // counters for LogIndividualData stuff 

        private: 
    };
}
