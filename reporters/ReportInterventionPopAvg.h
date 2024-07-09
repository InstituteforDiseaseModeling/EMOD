

#pragma once

#include "BaseTextReport.h"
#include "ReportFactory.h"
#include "ReportFilter.h"
#include "IReportInterventionDataAccess.h"

namespace Kernel
{
    struct InterventionStats : ReportInterventionData
    {
        float num_has;
        float num_instances;

        InterventionStats()
            : ReportInterventionData()
            , num_has(0.0f)
            , num_instances(0.0f)
        {
        }

        virtual void AddData( const ReportInterventionData& rData, bool alreadyHas )
        {
            num_instances += 1.0;
            if( !alreadyHas )
            {
                num_has += 1.0f;
            }
            ReportInterventionData::AddData( rData );
        }
    };

    class ReportInterventionPopAvg: public BaseTextReport
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportInterventionPopAvg, IReport )
    public:
        ReportInterventionPopAvg();
        virtual ~ReportInterventionPopAvg();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds ) override;
        virtual std::string GetHeader() const override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void LogNodeData( INodeContext * pNC ) override;
        virtual std::string GetReportName() const override;

    protected:
        ReportInterventionPopAvg( const std::string& rReportName );
        void UpdateStats( const std::string& rInterventionName,
                          ISupports* pIntervention,
                          std::set<std::string>& rInterventionNameSet );

        ReportFilter m_ReportFilter;
        bool m_IsValidTime;
        std::map<std::string, InterventionStats> m_Stats;
        std::set<std::string> m_NotSupported;
    };
}
