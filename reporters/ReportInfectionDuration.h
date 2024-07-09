

#pragma once

#include "BaseTextReport.h"
#include "ReportFactory.h"
#include "EventTrigger.h"
#include "BroadcasterObserver.h"

namespace Kernel
{
    class ReportInfectionDuration : public BaseTextReport
                                  , public IIndividualEventObserver
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportInfectionDuration, IReport )
    public:
        ReportInfectionDuration();
        virtual ~ReportInfectionDuration();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;

        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;

        // IObserver
        bool notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger ) override;

    protected:
        bool m_IsRegistered;
        float m_StartDay;
        float m_EndDay;
    };
}
