
#pragma once

#include <vector>

#include "BaseReportEventRecorder.h"
#include "ReportStatsByIP.h"

namespace Kernel
{
    struct INodeEventContext;
    class EventTriggerNodeFactory;

    class ReportEventRecorderNode : public BaseReportEventRecorder< INodeEventBroadcaster,
                                                                    INodeEventObserver,
                                                                    INodeEventContext,
                                                                    EventTriggerNode,
                                                                    EventTriggerNodeFactory >
    {
    public:
        GET_SCHEMA_STATIC_WRAPPER( ReportEventRecorderNode )

    public:
        static IReport* CreateReport();

    public:
        ReportEventRecorderNode();
        virtual ~ReportEventRecorderNode();

        // ------------
        // --- IReport
        // ------------
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;

        virtual std::string GetHeader() const override;

    protected:
        virtual void ConfigureOther( const Configuration* inputJson ) override;
        virtual std::string GetOtherData( INodeEventContext *context, const EventTriggerNode& trigger ) override;
        virtual std::string GetTime( INodeEventContext* pEntity ) const override;

        jsonConfigurable::tDynamicStringSet m_NodePropertiesToReport;
        jsonConfigurable::tDynamicStringSet m_StatsByIpKeyNames;
        ReportStatsByIP m_ReportStatsByIP;
    };
}
