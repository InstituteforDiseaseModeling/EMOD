/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BinnedReport.h"

namespace Kernel {
    class BinnedReportPy : public BinnedReport
    {
        public:
            static IReport* CreateReport();
            virtual ~BinnedReportPy();

            virtual void LogIndividualData( IIndividualHuman * individual );
            virtual void EndTimestep( float currentTime, float dt );

            virtual void postProcessAccumulatedData();

        protected:
            BinnedReportPy();

            virtual void initChannelBins();
            void clearChannelsBins();

            // channels specific to this particular report-type
    };
}
