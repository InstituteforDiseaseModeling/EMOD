/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BinnedReport.h"

namespace Kernel {

class BinnedReportTB : public BinnedReport
{
public:
    static IReport* CreateReport();
    virtual ~BinnedReportTB();

    virtual void LogIndividualData( Kernel::IIndividualHuman* individual );
    virtual void EndTimestep( float currentTime, float dt );

    virtual void postProcessAccumulatedData();

protected:
    BinnedReportTB();

    virtual void initChannelBins();
    void clearChannelsBins();

    // channels specific to this particular report-type
    float *active_infection_bins;
    float *MDR_active_infections_bins;
    float *infectiousness_bins;
    float *Tx_naive_infectiousness_bins;
    float *infectiousness_from_fast_bins;
    float *MDR_infectiousness_bins;
    float *active_from_fast_infection_bins;
    float *MDR_active_from_fast_infection_bins;
    float *disease_deaths_ontx_bins;

    float *active_smearpos_infection_bins;
};

}
