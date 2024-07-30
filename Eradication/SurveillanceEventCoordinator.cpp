
#include "stdafx.h"
#include "SurveillanceEventCoordinator.h"
#include "SimulationEventContext.h"
#include "InterventionFactory.h"
#include "IncidenceCounterSurveillance.h"
#include "ReportStatsByIP.h"

SETUP_LOGGING("SurveillanceEventCoordinator")

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- ResponderSurveillance
    // ------------------------------------------------------------------------

    ResponderSurveillance::ResponderSurveillance()
        : Responder()
        , m_RespondedEvent()
        , m_PercentageEventsToCount()
    {
    }

    ResponderSurveillance::~ResponderSurveillance()
    {
    }

    bool ResponderSurveillance::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Responded_Event", &m_RespondedEvent, SEC_Responded_Event_DESC_TEXT );

        initConfigTypeMap( "Percentage_Events_To_Count",
                           &m_PercentageEventsToCount,
                           SEC_Percentage_Events_To_Count_DESC_TEXT,
                           nullptr,
                           JsonConfigurable::empty_set,
                           "Threshold_Type",
                           "PERCENTAGE_EVENTS" );

        bool ret = Responder::Configure( inputJson );
        return ret;
    }

    void ResponderSurveillance::CheckConfiguration( const Configuration * inputJson )
    {
        //empty, no checks needed
    }

    bool ResponderSurveillance::Distribute( const std::vector<INodeEventContext*>& nodes, uint32_t numIncidences, uint32_t qualifyingPopulation )
    {
        bool distributed = Responder::Distribute( nodes, numIncidences, qualifyingPopulation );
        if( distributed && !m_RespondedEvent.IsUninitialized() )
        {
            m_sim->GetCoordinatorEventBroadcaster()->TriggerObservers( m_Parent, m_RespondedEvent );
        }
        return distributed;
    }

    // ------------------------------------------------------------------------
    // --- SurveillanceEventCoordinator
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(SurveillanceEventCoordinator)

    BEGIN_QUERY_INTERFACE_DERIVED( SurveillanceEventCoordinator, IncidenceEventCoordinator )
        HANDLE_INTERFACE( IEventCoordinatorEventContext )
        HANDLE_INTERFACE( ISurveillanceReporting )
    END_QUERY_INTERFACE_DERIVED( SurveillanceEventCoordinator, IncidenceEventCoordinator )

    SurveillanceEventCoordinator::SurveillanceEventCoordinator()
        : IncidenceEventCoordinator( new IncidenceCounterSurveillance(), new ResponderSurveillance() )
        , m_StopTriggerConditionList()
        , m_StartTriggerConditionList()
        , m_IsActive( false )
        , m_IsStarting( false )
        , m_IsStopping( false )
        , m_Duration( -1.0f )
        , m_DurationExpired ( false )
    {
        initSimTypes( 1, "*" );
        m_CoordinatorName = "SurveillanceEventCoordinator";
    }

    SurveillanceEventCoordinator::~SurveillanceEventCoordinator()
    {
    }


    bool SurveillanceEventCoordinator::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Start_Trigger_Condition_List", &m_StartTriggerConditionList, SEC_Start_Trigger_Condition_List_DESC_TEXT );
        initConfigTypeMap( "Stop_Trigger_Condition_List", &m_StopTriggerConditionList, SEC_Stop_Trigger_Condition_List_DESC_TEXT );
        initConfigTypeMap( "Duration", &m_Duration, DEC_Duration_DESC_TEXT, -1.0f, FLT_MAX, -1.0f );

        bool retValue = IncidenceEventCoordinator::Configure( inputJson );

        if( retValue && !JsonConfigurable::_dryrun )
        {
            CheckConfigurationTriggers();
            IncidenceCounterSurveillance* p_ics = static_cast<IncidenceCounterSurveillance*>(m_pIncidenceCounter);
            ResponderSurveillance* p_rs = static_cast<ResponderSurveillance*>(m_pResponder);
            p_ics->SetPercentageEventsToCount( p_rs->GetPercentageEventsToCount() );
        }

        return retValue;
    }

    void SurveillanceEventCoordinator::CheckConfigurationTriggers()
    {
        //check if the same trigger is defined as start and stop condition
        for( EventTriggerCoordinator& etc : m_StopTriggerConditionList )
        {
            auto found = find( m_StartTriggerConditionList.begin(), m_StartTriggerConditionList.end(), etc );
            if( found != m_StartTriggerConditionList.end() )
            {
                std::ostringstream msg;
                msg << "In Coordinator \'" << m_CoordinatorName << "\' stop trigger \'" << etc.ToString() << "\' is already defined in Start_Trigger_Condition_List.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
    }

    void SurveillanceEventCoordinator::SetContextTo( ISimulationEventContext *isec )
    {
        IncidenceEventCoordinator::SetContextTo( isec );
        m_pResponder->SetContextTo( m_Parent, this );

        Register();
    }

    void SurveillanceEventCoordinator::Update( float dt )
    {        
        /*
        Methods are called in the following order:
        1) Update()         - Set state of object based on notifications from previous time step
        2) notifyOnEvent()  - check for new events
        3) UpdateNodes()    - execute actions like e.g. responing
        */
        if( m_Duration != -1.0f )
        {
            if( m_Duration <= 0  )
            {
                m_DurationExpired = true;
            }
            m_Duration -= dt;
        }
        if( m_IsStarting )
        {
            m_IsStarting = false;
            m_IsActive = true;
            LOG_INFO_F( "%s: Starting, so Registering for events\n", m_CoordinatorName.c_str() );
            for( auto pNEC : m_CachedNodes )
            {
                m_pIncidenceCounter->RegisterForEvents( pNEC );
            }
            m_pIncidenceCounter->StartCounting();
        }
        if( m_IsStopping )
        {
            m_IsStopping = false;
            m_IsActive = false;
            LOG_INFO_F( "%s: Stopping, so Unregistering for events\n", m_CoordinatorName.c_str() );
            for( auto pNEC : m_CachedNodes )
            {
                m_pIncidenceCounter->UnregisterForEvents( pNEC );
            }
        }

        if( !m_IsActive ) return;
        m_pIncidenceCounter->Update( dt );
    }

    void SurveillanceEventCoordinator::Register()
    {
        //Register Start and Stop events
        for( EventTriggerCoordinator& ect : m_StartTriggerConditionList)
        {
            m_Parent->GetCoordinatorEventBroadcaster()->RegisterObserver( this, ect );
            LOG_INFO_F( "%s: registered Start_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str() );
        }
        for( EventTriggerCoordinator& ect : m_StopTriggerConditionList)
        {
            m_Parent->GetCoordinatorEventBroadcaster()->RegisterObserver( this, ect );
            LOG_INFO_F( "%s: registered Stop_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str() );
        }
    }

    bool SurveillanceEventCoordinator::notifyOnEvent( IEventCoordinatorEventContext *pEntity, const EventTriggerCoordinator& trigger )
    {
        auto it_start = find( m_StartTriggerConditionList.begin(), m_StartTriggerConditionList.end(), trigger );
        if ( it_start != m_StartTriggerConditionList.end() )
        {
            LOG_INFO_F( "%s: notifyOnEvent received Start: %s\n", m_CoordinatorName.c_str(), trigger.ToString().c_str() );
            m_IsStarting = true;           
        }
        else
        {
            LOG_INFO_F( "%s: notifyOnEvent received Stop: %s\n", m_CoordinatorName.c_str(), trigger.ToString().c_str() );
            m_IsStopping = true;
        }
        return true;
    }

    void SurveillanceEventCoordinator::Unregister()
    {
        for ( EventTriggerCoordinator& ect : m_StartTriggerConditionList )
        {
            m_Parent->GetCoordinatorEventBroadcaster()->UnregisterObserver( this, ect );
            LOG_INFO_F( "%s: Unregistered Start_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str() );
        }
        for ( EventTriggerCoordinator& ect : m_StopTriggerConditionList )
        {
            m_Parent->GetCoordinatorEventBroadcaster()->UnregisterObserver( this, ect );
            LOG_INFO_F( "%s: Unregistered Stop_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str() );
        }
    }

    bool SurveillanceEventCoordinator::IsFinished()
    {
        if( m_DurationExpired )
        {
            Unregister();
            m_IsExpired = true;
            IncidenceEventCoordinator::IsFinished();
        }
        return m_DurationExpired;
    }

    void SurveillanceEventCoordinator::ConsiderResponding()
    {
        if (m_pIncidenceCounter->IsDoneCounting())
        {
            uint32_t num_incidences = m_pIncidenceCounter->GetCount();
            uint32_t num_qualifying_pop = m_pIncidenceCounter->GetCountOfQualifyingPopulation( m_CachedNodes );
            m_pResponder->Distribute( m_CachedNodes, num_incidences, num_qualifying_pop );
            m_pIncidenceCounter->StartCounting();
        }
    }

    void SurveillanceEventCoordinator::AddNode( const suids::suid& node_suid )
    {
        INodeEventContext* pNEC = m_Parent->GetNodeEventContext( node_suid );
        m_CachedNodes.push_back( pNEC );

        //don't register counter events now
        //m_pIncidenceCounter->RegisterForEvents( pNEC );
    }

    void SurveillanceEventCoordinator::UpdateNodes(float dt)
    {
        if( !m_IsActive ) return; 
        ConsiderResponding();
    }

    void SurveillanceEventCoordinator::CollectStats( ReportStatsByIP& rStats )
    {
        for( auto p_nec : m_CachedNodes )
        {
            if( m_pIncidenceCounter->IsNodeQualified( p_nec ) )
            {
                rStats.CollectDataFromNode( p_nec,
                                            m_pIncidenceCounter->GetIndividualQualifiedFunction() );
            }
        }
    }

    uint32_t SurveillanceEventCoordinator::GetNumCounted() const
    {
        return m_pIncidenceCounter->GetCount();
    }

    float SurveillanceEventCoordinator::GetCurrentActionThreshold() const
    {
        return m_pResponder->GetCurrentAction()->GetThreshold();
    }
}