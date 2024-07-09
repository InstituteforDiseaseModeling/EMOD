
#pragma once

#include "BinnedReport.h"
#include "IReportMalariaDiagnostics.h"

namespace Kernel {

class BinnedReportMalaria : public BinnedReport, public IReportMalariaDiagnostics
{
public:
    // needed for IReportMalariaDiagnostics
    DECLARE_QUERY_INTERFACE()
    IMPLEMENT_NO_REFERENCE_COUNTING()

public:
    static IReport* CreateReport();
    virtual ~BinnedReportMalaria();

    // BinnedReport
    virtual void LogIndividualData( IIndividualHuman* individual ) override;
    virtual void EndTimestep( float currentTime, float dt ) override;
    virtual void postProcessAccumulatedData() override;

    // IReportMalariaDiagnostics
    virtual void SetDetectionThresholds( const std::vector<float>& rDetectionThresholds ) override;

protected:
    BinnedReportMalaria();

    virtual void initChannelBins() override;
    void clearChannelsBins();

    std::vector<float> m_DetectionThresholds;

    // channels specific to this particular report-type
    std::vector<std::pair<std::string,float*>> infection_detected_bins;

    float *mean_parasitemia_bins;
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
