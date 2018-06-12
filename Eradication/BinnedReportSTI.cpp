/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "BinnedReportSTI.h"

#include <map>

#include "Environment.h"
#include "Exceptions.h"
#include "Sugar.h"

using namespace std;
using namespace json;

namespace Kernel
{

    SETUP_LOGGING( "BinnedReportSTI" )

    IReport*
    BinnedReportSTI::CreateReport()
    {
        return new BinnedReportSTI();
    }

    // Derived constructor calls base constructor to initialized reduced timesteps etc. 
    BinnedReportSTI::BinnedReportSTI()
        : BinnedReport()
    {
    }

    BinnedReportSTI::~BinnedReportSTI()
    {
    }

    void BinnedReportSTI::postProcessAccumulatedData()
    {
        BinnedReport::postProcessAccumulatedData();
    }
}
