/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BinnedReport.h"

namespace Kernel {

class BinnedReportMalaria : public BinnedReport
{
public:
    static IReport* CreateReport();
    virtual ~BinnedReportMalaria();

    virtual void LogIndividualData( IIndividualHuman* individual );
    virtual void EndTimestep( float currentTime, float dt );

    virtual void postProcessAccumulatedData();

protected:
    BinnedReportMalaria();

    virtual void initChannelBins();
    void clearChannelsBins();

    // channels specific to this particular report-type
    float *parasite_positive_bins;
    float *fever_positive_bins;
    float *mean_parasitemia_bins;
    float *new_diagnostic_prev_bins;
    float *new_clinical_cases_bins;
    float *new_severe_cases_bins;

    float *msp_variant_bins;    // sum of variant fractions
    float *nonspec_variant_bins;
    float *pfemp1_variant_bins;

    float *ss_msp_variant_bins; // sum-of-squares
    float *ss_nonspec_variant_bins;
    float *ss_pfemp1_variant_bins;
};

}
