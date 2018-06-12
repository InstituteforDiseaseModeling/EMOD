/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "EventCoordinator.h"
#include "Configure.h"
#include "DemographicRestrictions.h"
#include "DurationDistribution.h"
#include "Interventions.h"
#include "EventTrigger.h"
#include "NodeEventContext.h"
#include "JsonConfigurableCollection.h"

namespace Kernel
{
    struct INodeEventContext;
    struct IIndividualHumanEventContext;
    struct INodeTriggeredInterventionConsumer;

    ENUM_DEFINE( ThresholdType,
        ENUM_VALUE_SPEC( COUNT,      1 )
        ENUM_VALUE_SPEC( PERCENTAGE, 2 ) )

    // ------------------------------------------------------------------------
    // --- Action
    // ------------------------------------------------------------------------

    class Action : public JsonConfigurable
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        Action();
        virtual ~Action();

        // JsonConfigurable methods
        virtual bool Configure( const Configuration * inputJson ) override;

        // Other methods
        float GetThreshold() const;
        const EventTrigger& GetEventToBroadcast() const;

    private:
        float        m_Threshold;
        EventTrigger m_EventToBroadcast;
    };

    // ------------------------------------------------------------------------
    // --- ActionList
    // ------------------------------------------------------------------------

    class ActionList : public JsonConfigurableCollection<Action>
    {
    public:
        ActionList();
        virtual ~ActionList();

        virtual void CheckConfiguration() override;

    protected:
        virtual Action* CreateObject() override;
    };

    // ------------------------------------------------------------------------
    // --- Responder
    // ------------------------------------------------------------------------

    class Responder : public JsonConfigurable, public IVisitIndividual
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        Responder();
        virtual ~Responder();

        // JsonConfigurable methods
        virtual bool Configure( const Configuration * inputJson ) override;

        // IVisitIndividual methods
        virtual bool visitIndividualCallback( IIndividualHumanEventContext *ihec, float & incrementalCostOut, ICampaignCostObserver * pICCO );

        // Other methods
        void Distribute( const std::vector<INodeEventContext*>& nodes, uint32_t numIncidences, uint32_t qualifyingPopulation );

    private:
        float CalculateIndidence( uint32_t numIncidences, uint32_t qualifyingPopulation ) const;
        Action* GetAction( float value );

        ThresholdType::Enum m_ThresholdType;
        ActionList m_ActionList;
        Action* m_pCurrentAction;
    };

    // ------------------------------------------------------------------------
    // --- IncidenceCounter
    // ------------------------------------------------------------------------

    class IncidenceCounter : public JsonConfigurable, public IIndividualEventObserver
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        IncidenceCounter();
        virtual ~IncidenceCounter();

        virtual bool Configure( const Configuration * inputJson ) override;

        // IIndividualEventObserver methods
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context,
                                    const EventTrigger& StateChange ) override;

        // Other methods
        uint32_t GetCount() const;
        uint32_t GetCountOfQualifyingPopulation( const std::vector<INodeEventContext*>& rNodes );
        void StartCounting();
        void Update( float dt );
        bool IsDoneCounting() const;
        void RegisterForEvents( INodeEventContext* pNEC );
        void UnregisterForEvents( INodeEventContext* pNEC );
        int32_t GetCountEventsForNumTimeSteps() const;

    private:
        INodeTriggeredInterventionConsumer* GetNodeTriggeredConsumer( INodeEventContext* pNEC );

        uint32_t m_Count;
        int32_t m_CountEventsForNumTimeSteps;
        int32_t m_NumTimeStepsCounted;
        bool m_IsDoneCounting;
        DemographicRestrictions   m_DemographicRestrictions;
        std::vector<EventTrigger> m_TriggerConditionList;
        PropertyRestrictions<NPKey, NPKeyValue, NPKeyValueContainer> m_NodePropertyRestrictions;

    };

    // ------------------------------------------------------------------------
    // --- IncidenceEventCoordinator
    // ------------------------------------------------------------------------

    class IncidenceEventCoordinator : public IEventCoordinator, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT( EventCoordinatorFactory, IncidenceEventCoordinator, IEventCoordinator )
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        IncidenceEventCoordinator();
        virtual ~IncidenceEventCoordinator();

        virtual bool Configure( const Configuration * inputJson ) override;
        virtual QuickBuilder GetSchema() override;

        // IEventCoordinator methods
        virtual void SetContextTo( ISimulationEventContext *isec ) override;
        virtual void CheckStartDay( float campaignStartDay ) const override { };
        virtual void AddNode( const suids::suid& suid ) override;
        virtual void Update( float dt ) override;
        virtual void UpdateNodes( float dt ) override;
        virtual bool IsFinished() override;

        // Other methods - for testing
        int GetNumTimeStepsInRep() const { return m_NumTimestepsInRep; }
        int GetNumReps() const { return m_NumReps; }
        int GetEventCount() const { return m_IncidenceCounter.GetCount(); }
        uint32_t GetQualifyingCount() { return m_IncidenceCounter.GetCountOfQualifyingPopulation( m_CachedNodes ); }

    protected:
        ISimulationEventContext*        m_Parent;
        std::vector<INodeEventContext*> m_CachedNodes;
        bool                            m_IsExpired;
        bool                            m_HasBeenDistributed;
        int32_t                         m_NumReps;
        int32_t                         m_NumTimestepsBetweenReps;
        int32_t                         m_NumTimestepsInRep;
        IncidenceCounter                m_IncidenceCounter;
        Responder                       m_Responder;
    };
}

