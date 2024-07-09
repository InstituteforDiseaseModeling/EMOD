
#pragma once

#include "BaseChannelReport.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{

class ReportPluginAgeAtInfectionHistogram : public BaseChannelReport
{
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportPluginAgeAtInfectionHistogram, IReport )
#endif
public:
    ReportPluginAgeAtInfectionHistogram();
    virtual ~ReportPluginAgeAtInfectionHistogram() { }

    // ISupports
    virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
    virtual int32_t AddRef() override { return BaseChannelReport::AddRef(); }
    virtual int32_t Release() override { return BaseChannelReport::Release(); }

    virtual void BeginTimestep() override;
    virtual void EndTimestep( float currentTime, float dt ) override;
    virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override { return true ; } ;
    virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
    virtual void Reduce() override;
    virtual void Finalize() override;

protected:
    virtual bool Configure( const Configuration* config) override;
    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
    virtual void postProcessAccumulatedData() override;
    int  GetAgeBin(double age);

    float time_since_last_report;
    float reporting_interval_in_years;

    std::vector< float > temp_binned_accumulated_counts;
    std::vector< std::vector< float > > binned_accumulated_counts;

    std::vector< float > age_bin_upper_edges_in_years;

private:
};

}
