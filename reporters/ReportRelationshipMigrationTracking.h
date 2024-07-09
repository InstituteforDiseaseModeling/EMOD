
#pragma once

#include "BaseTextReportEvents.h"

#include "ReportFilter.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    class ReportRelationshipMigrationTracking : public BaseTextReportEvents
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportRelationshipMigrationTracking, IReport )
#endif
    public:
        ReportRelationshipMigrationTracking();
        virtual ~ReportRelationshipMigrationTracking();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReportEvents::AddRef(); }
        virtual int32_t Release() override { return BaseTextReportEvents::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds ) override;
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;

        virtual std::string GetReportName() const override;
        virtual std::string GetHeader() const override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const EventTrigger& trigger ) override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual void Reduce() override;

        //BaseTextReportEvents
        virtual bool IsValidNode( INodeEventContext* pNEC ) const override;

    private:
        struct MigrationData
        {
            MigrationData() : age_years(-1.0), gender(-1), node_id(-1), migration_type_str() {}

            float age_years ;
            int gender ;
            uint32_t node_id ;
            std::string migration_type_str ;
        };
        float m_EndTime ;
        std::map<long,MigrationData> m_MigrationDataMap ;

        bool m_IsCollectingData;
        ReportFilter m_ReportFilter;
    };
}
