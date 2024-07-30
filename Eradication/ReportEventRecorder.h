
#pragma once

#include <vector>

#include "BaseReportEventRecorder.h"
#include "Properties.h"
#include "ReportFilter.h"

namespace Kernel
{
    class EventTriggerFactory;

    class ReportEventRecorder : public BaseReportEventRecorder< IIndividualEventBroadcaster,
                                                                IIndividualEventObserver,
                                                                IIndividualHumanEventContext,
                                                                EventTrigger,
                                                                EventTriggerFactory >
    {
    public:
        GET_SCHEMA_STATIC_WRAPPER( ReportEventRecorder )

    public:
        static IReport* CreateReport();

    public:
        ReportEventRecorder( bool useYear=false );
        virtual ~ReportEventRecorder();

        // ------------
        // --- IReport
        // ------------
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds ) override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger ) override;

        virtual std::string GetHeader() const override;

    protected:
        virtual void ConfigureOther( const Configuration* inputJson ) override;
        virtual void CheckOther( const Configuration* inputJson ) override;
        virtual std::string GetOtherData( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;
        virtual std::string GetTime( IIndividualHumanEventContext* pEntity ) const override;

        jsonConfigurable::tDynamicStringSet m_PropertiesToReport;
        std::string m_PropertyChangeIPKeyString;
        IPKey m_PropertyChangeIPKey;
        ReportFilter m_ReportFilter;
    };
}
