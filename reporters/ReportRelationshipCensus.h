

#pragma once

#include "BaseTextReportEvents.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    class ReportRelationshipCensus : public BaseTextReportEvents
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportRelationshipCensus, IReport )
#endif
    public:
        ReportRelationshipCensus();
        virtual ~ReportRelationshipCensus();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReportEvents::AddRef(); }
        virtual int32_t Release() override { return BaseTextReportEvents::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;

        virtual std::string GetHeader() const override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const Kernel::EventTrigger &trigger ) override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual void Reduce() override;
        virtual std::string GetReportName() const override;

    private:
        std::string m_ReportName;
        float m_StartYear;
        float m_EndYear;
        float m_ReportingIntervalYears;
        float m_IntervalTimerDays;
        bool m_IsCollectingData;
        bool m_FirstDataCollection;
    };
}
