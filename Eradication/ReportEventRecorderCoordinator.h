
#pragma once

#include <vector>

#include "BaseReportEventRecorder.h"

namespace Kernel
{
    class ICoordinatorEventContext;
    class EventTriggerCoordinatorFactory;

    class ReportEventRecorderCoordinator : public BaseReportEventRecorder< ICoordinatorEventBroadcaster,
                                                                           ICoordinatorEventObserver,
                                                                           IEventCoordinatorEventContext,
                                                                           EventTriggerCoordinator,
                                                                           EventTriggerCoordinatorFactory >
    {
    public:
        GET_SCHEMA_STATIC_WRAPPER( ReportEventRecorderCoordinator )

    public:
        static IReport* CreateReport();

    public:
        ReportEventRecorderCoordinator();
        ReportEventRecorderCoordinator( const std::string& rReportName );
        virtual ~ReportEventRecorderCoordinator();

        // ------------
        // --- IReport
        // ------------
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;

        virtual std::string GetHeader() const override;

    protected:
        virtual std::string GetOtherData( IEventCoordinatorEventContext *context, const EventTriggerCoordinator& trigger ) override;
        virtual std::string GetTime( IEventCoordinatorEventContext* pEntity ) const override;
    };
}
