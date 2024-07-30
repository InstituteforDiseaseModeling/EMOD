
#pragma once

#include <list>
#include <map>
#include <vector>
#include <string>
#include <fstream>

#include "SpatialReportVector.h"
#include "IReportMalariaDiagnostics.h"

namespace Kernel {

class SpatialReportMalaria : public SpatialReportVector, public IReportMalariaDiagnostics
{
    GET_SCHEMA_STATIC_WRAPPER(SpatialReportMalaria)
public:

    // needed for IReportMalariaDiagnostics
    DECLARE_QUERY_INTERFACE()
    IMPLEMENT_NO_REFERENCE_COUNTING()

    static IReport* CreateReport();
    virtual ~SpatialReportMalaria() { }

    // SpatialReportVector
    virtual void Initialize( unsigned int nrmSize ) override;
    virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
    virtual void LogIndividualData( IIndividualHuman* individual ) override;

    // IReportMalariaDiagnostics
    virtual void SetDetectionThresholds( const std::vector<float>& rDetectionThresholds ) override;

protected:
    SpatialReportMalaria();

    virtual void postProcessAccumulatedData() override;

    virtual void populateChannelInfos(tChanInfoMap &channel_infos) override;

    std::vector<ChannelInfo> prevalence_by_diagnostic_info;
    ChannelInfo mean_parasitemia_info;
    ChannelInfo new_clinical_cases_info;
    ChannelInfo new_severe_cases_info;

    std::vector<float> m_DetectionThresholds;
    std::vector<float> m_Detected;
    float              m_MeanParasitemia;
};

}
