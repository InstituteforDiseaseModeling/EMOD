/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BinnedReport.h"

namespace Kernel {

class BinnedReportTyphoid : public BinnedReport
{
public:
    static IReport* CreateReport();
    virtual ~BinnedReportTyphoid();

    virtual void LogIndividualData( IIndividualHuman * individual ) override;
    virtual void EndTimestep( float currentTime, float dt ) override;

    virtual void postProcessAccumulatedData() override;

protected:
    BinnedReportTyphoid();

    virtual void initChannelBins();
    void clearChannelsBins();

    float *carrier_bins;
    float *subclinical_bins;
    float *acute_bins;
    // channels specific to this particular report-type
};

}
