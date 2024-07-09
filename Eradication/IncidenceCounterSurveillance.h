
#pragma once
#include "IncidenceEventCoordinator.h"
#include "BroadcasterObserver.h"
#include "SimulationEventContext.h"

namespace Kernel
{
    ENUM_DEFINE(CounterType,
        ENUM_VALUE_SPEC(PERIODIC, 1))

    class IncidenceCounterSurveillance 
        : public IncidenceCounter, public ICoordinatorEventObserver, public INodeEventObserver
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        IncidenceCounterSurveillance();
        virtual ~IncidenceCounterSurveillance();
        virtual bool Configure( const Configuration * inputJson ) override;
        virtual void ConfigureTriggers( const Configuration * inputJson ) override;
        virtual void CheckConfigurationTriggers() override;
        virtual void Update( float dt ) override;
        virtual void RegisterForEvents( ISimulationEventContext* context ) override;
        virtual void UnregisterForEvents( ISimulationEventContext * context ) override;
        virtual void RegisterForEvents( INodeEventContext* pNEC ) override;
        virtual void UnregisterForEvents( INodeEventContext* pNEC ) override;

        void SetPercentageEventsToCount( const std::vector<std::string>& rPercentageEvents );

    protected:
        virtual void StartCounting() override;
        virtual bool IsDoneCounting() const override;        

        // IIndividualEventObserver methods
        virtual bool notifyOnEvent( IEventCoordinatorEventContext *pEntity, const EventTriggerCoordinator& trigger ) override;
        virtual uint32_t GetCountOfQualifyingPopulation( const std::vector<INodeEventContext*>& rNodes ) override;
        virtual bool notifyOnEvent( INodeEventContext *pEntity, const EventTriggerNode& trigger ) override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext * pEntity, const EventTrigger & trigger ) override;

    private:
        float m_CounterPeriod;
        float m_CounterPeriod_current;
        CounterType::Enum     m_CounterType;
        EventType::Enum       m_CounterEventType;

        std::vector<std::string>             m_TriggerConditionList;
        std::vector<EventTriggerNode>        m_TriggerConditionListNode;
        std::vector<EventTriggerCoordinator> m_TriggerConditionListCoordinator;

        std::vector<EventTrigger>            m_PercentageEventsToCountIndividual;
        std::vector<EventTriggerNode>        m_PercentageEventsToCountNode;
        std::vector<EventTriggerCoordinator> m_PercentageEventsToCountCoordinator;
        uint32_t m_PercentageEventsCounted;
    };
}