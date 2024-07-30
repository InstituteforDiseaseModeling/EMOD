
#pragma once

#include "Report.h"

namespace Kernel {

class ReportVector : public Report
{
public:
    ReportVector();
    virtual ~ReportVector() {};

    static IReport* ReportVector::CreateReport() { return new ReportVector(); }

    virtual void LogNodeData( Kernel::INodeContext * pNC ) override;

protected:
    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
    virtual void postProcessAccumulatedData() override;

    virtual void AddSEIRChannels() override {};
    virtual void AddSEIRWUnits( std::map<std::string, std::string> &units_map ) override {}
    virtual void UpdateSEIRW( const Kernel::IIndividualHuman* individual, float monte_carlo_weight ) override {}
    virtual void AccumulateSEIRW() override {}
    virtual void NormalizeSEIRWChannels() override {}

    ChannelID adult_vectors_id;
    ChannelID infectious_vectors_id;
    ChannelID daily_EIR_id;
    ChannelID daily_HBR_id;
};

}
