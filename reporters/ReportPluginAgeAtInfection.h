
#pragma once

#include "BaseChannelReport.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
class RANDOMBASE;

class ReportPluginAgeAtInfection : public BaseChannelReport
{
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportPluginAgeAtInfection, IReport )
#endif
public:

    ReportPluginAgeAtInfection();
    virtual ~ReportPluginAgeAtInfection();

    // ISupports
    virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
    virtual int32_t AddRef() override { return BaseChannelReport::AddRef(); }
    virtual int32_t Release() override { return BaseChannelReport::Release(); }

    virtual void EndTimestep( float currentTime, float dt ) override;
    virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override { return true ; } ;
    virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
    virtual void Reduce() override;
    virtual void Finalize() override;

protected:
    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
    virtual void postProcessAccumulatedData() override;
    float timestep;
    std::vector<float> ages;
    float sampling_ratio;

    std::map< unsigned int, std::vector<float> > time_age_map;
    Kernel::RANDOMBASE* m_pRNG;

private:
};

}
