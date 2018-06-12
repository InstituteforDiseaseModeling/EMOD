/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
