/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#pragma once

#include "IncidenceEventCoordinator.h"
#include "BroadcasterObserver.h"
#include "EventTriggerCoordinator.h"
#include "ISurveillanceReporting.h"

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- ResponderSurveillance
    // ------------------------------------------------------------------------

    class ResponderSurveillance : public Responder
    {
    public:
        ResponderSurveillance();
        virtual ~ResponderSurveillance();

        // Responder methods
        virtual bool Configure( const Configuration * inputJson ) override;
        virtual void CheckConfiguration( const Configuration * inputJson ) override;
        virtual bool Distribute( const std::vector<INodeEventContext*>& nodes, uint32_t numIncidences, uint32_t qualifyingPopulation );

        const std::vector<std::string>& GetPercentageEventsToCount() const { return m_PercentageEventsToCount; }

    private:
        EventTriggerCoordinator m_RespondedEvent;
        std::vector<std::string> m_PercentageEventsToCount;
    };

    // ------------------------------------------------------------------------
    // --- SurveillanceEventCoordinator
    // ------------------------------------------------------------------------

    class SurveillanceEventCoordinator : public IncidenceEventCoordinator
                                       , public ICoordinatorEventObserver
                                       , public IEventCoordinatorEventContext
                                       , public ISurveillanceReporting
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, SurveillanceEventCoordinator, IEventCoordinator)
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        SurveillanceEventCoordinator();
        virtual ~SurveillanceEventCoordinator();

        virtual bool Configure(const Configuration * inputJson) override;       
        virtual bool notifyOnEvent( IEventCoordinatorEventContext *pEntity, const EventTriggerCoordinator& trigger ) override;
        void Register();
        void Unregister();
        virtual void SetContextTo( ISimulationEventContext *isec ) override;
        virtual void Update(float dt) override;

        // IEventCoordinatorEventContext
        virtual const std::string& GetName() const override;
        virtual const IdmDateTime& GetTime() const override;
        virtual IEventCoordinator* GetEventCoordinator() override { return this; };
        virtual IEventCoordinatorEventContext* GetEventContext() override { return this; }

        virtual bool IsFinished() override;
        virtual void ConsiderResponding();
        virtual void UpdateNodes( float dt ) override;
        ISimulationEventContext* GetSimulationContext() { return m_Parent; };

        // ISurveillanceReporting
        virtual void CollectStats( ReportStatsByIP& rStats ) override;
        virtual uint32_t GetNumCounted() const override;
        virtual float GetCurrentActionThreshold() const override;

    protected:
        virtual void ConfigureRepetitions( const Configuration * inputJson ) override { };
        virtual void CheckConfigureRepetitions() override {}
        void CheckConfigurationTriggers();

        std::vector<EventTriggerCoordinator> m_StartTriggerConditionList;
        std::vector<EventTriggerCoordinator> m_StopTriggerConditionList;

    private:
        bool m_IsActive;
        bool m_IsStarting;
        bool m_IsStopping;
        float m_Duration;
        float m_CounterPeriod;
        bool m_DurationExpired;
        std::string m_CoordinatorName;
        EventTriggerCoordinator m_RespondedEvent;
    };




}