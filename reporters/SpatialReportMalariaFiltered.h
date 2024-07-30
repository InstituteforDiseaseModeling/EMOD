
#pragma once

#include <map>

#include "BaseTextReport.h"
#include "SpatialReportMalaria.h"
#include "ReportFilter.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    class SpatialReportMalariaFiltered : public SpatialReportMalaria
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, SpatialReportMalariaFiltered, IReport )
#endif
    public:
        SpatialReportMalariaFiltered();
        virtual ~SpatialReportMalariaFiltered();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return SpatialReportMalaria::AddRef(); }
        virtual int32_t Release() override { return SpatialReportMalaria::Release(); }

        // SpatialReportMalaria
        virtual bool Configure( const Configuration* ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& nodeIds_demographics ) override;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual void BeginTimestep() override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogNodeData( INodeContext* pNC ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual void Reduce() override;
        virtual std::string GetReportName() const override;

    private:
        // SpatialReport methods
        virtual const char* GetChannelsDependsOn() const override;

        virtual void WriteHeaderParameters( std::ofstream* file ) override;
        virtual void postProcessAccumulatedData() override;
        virtual void WriteData( ChannelDataMap& rChannelDataMap ) override;
        virtual void ClearData() override;

        ReportFilter m_ReportFilter;
        float m_ReportingInterval;
        float m_IntervalTimer;
        float m_TimeElapsed;
        float m_Dt;
        bool m_IsValidDay;
        bool m_HasValidNode;
        ChannelDataMap m_FilteredChannelDataMap;
    };
}
