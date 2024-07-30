
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
