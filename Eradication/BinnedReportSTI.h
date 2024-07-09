
#pragma once

#include "BinnedReport.h"

namespace Kernel
{

    class BinnedReportSTI : public BinnedReport
    {
    public:
        static IReport* CreateReport();
        virtual ~BinnedReportSTI();

        virtual void postProcessAccumulatedData();

    protected:
        BinnedReportSTI();
    };

}
