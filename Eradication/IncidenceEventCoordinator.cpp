/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "IncidenceEventCoordinator.h"
#include "InterventionFactory.h"
#include "Environment.h"
#include "RANDOM.h"
#include "EventTrigger.h"
#include "NodeEventContext.h"
#include "INodeContext.h"
#include "IndividualEventContext.h"
#include "IIndividualHuman.h"

SETUP_LOGGING( "IncidenceEventCoordinator" )


namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- Action
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY( Action )
    END_QUERY_INTERFACE_BODY( Action )

    Action::Action()
        : JsonConfigurable()
        , m_Threshold( 0 )
        , m_EventToBroadcast()
    {
    }

    Action::~Action()
    {
    }

    bool Action::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Threshold", &m_Threshold, ICE_Action_Threshold_DESC_TEXT, 0.0, FLT_MAX, 0.0 );

        initConfigTypeMap( "Event_To_Broadcast", &m_EventToBroadcast, ICE_Action_Event_To_Broadcast_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_EventToBroadcast.IsUninitialized() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "You must define Event_To_Broadcast." );
            }
        }
        return ret;
    }

    float Action::GetThreshold() const
    {
        return m_Threshold;
    }

    const EventTrigger& Action::GetEventToBroadcast() const
    {
        return m_EventToBroadcast;
    }

    // ------------------------------------------------------------------------
    // --- ActionList
    // ------------------------------------------------------------------------

    ActionList::ActionList()
        : JsonConfigurableCollection("ActionList")
    {
    }

    ActionList::~ActionList()
    {
    }

    bool GreaterThan( Action* pLeft, Action* pRight )
    {
        return (pLeft->GetThreshold() > pRight->GetThreshold());
    }

    void ActionList::CheckConfiguration()
    {
        if( Size() == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "At least one action must be defined for the IncidenceEventCoordinator.");
        }

        // sort into decending order
        std::sort( m_Collection.begin(), m_Collection.end(), GreaterThan );

        // Check if any of the thresholds equal another
        for( int i = 1; i < m_Collection.size(); ++i )
        {
            if( m_Collection[ i - 1 ]->GetThreshold() == m_Collection[ i ]->GetThreshold() )
            {
                std::stringstream ss;
                ss << "More than one action has a Threshold equal to " << m_Collection[ i ]->GetThreshold() << ".  Threshold values must be unique.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }

    Action* ActionList::CreateObject()
    {
        Action* p_act = new Action();
        return p_act;
    }

    // ------------------------------------------------------------------------
    // --- Responder
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY( Responder )
    END_QUERY_INTERFACE_BODY( Responder )

    Responder::Responder()
        : JsonConfigurable()
        , m_ThresholdType(ThresholdType::COUNT)
        , m_ActionList()
        , m_pCurrentAction( nullptr )
    {
    }

    Responder::~Responder()
    {
    }

    bool Responder::Configure( const Configuration * inputJson )
    {
        initConfig( "Threshold_Type", m_ThresholdType, inputJson, MetadataDescriptor::Enum( "ThresholdType", ICE_Threshold_Type_DESC_TEXT, MDD_ENUM_ARGS( ThresholdType ) ) );

        initConfigComplexType( "Action_List", &m_ActionList, ICE_Action_List_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_ActionList.CheckConfiguration();
        }
        return ret;
    }

    bool Responder::visitIndividualCallback( IIndividualHumanEventContext *ihec, float & incrementalCostOut, ICampaignCostObserver * pICCO )
    {
        release_assert( m_pCurrentAction != nullptr );

        INodeTriggeredInterventionConsumer* broadcaster = nullptr;
        if( s_OK != ihec->GetNodeEventContext()->QueryInterface( GET_IID( INodeTriggeredInterventionConsumer ), (void**)&broadcaster ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "ihec->GetNodeEventContext()->GetNodeEventContext()",
                                           "INodeTriggeredInterventionConsumer", 
                                           "INodeEventContext" );
        }
        broadcaster->TriggerNodeEventObservers( ihec, m_pCurrentAction->GetEventToBroadcast() );

        return true;
    }

    void Responder::Distribute( const std::vector<INodeEventContext*>& nodes, uint32_t numIncidences, uint32_t qualifyingPopulation )
    {
        float incidence_value = CalculateIndidence( numIncidences, qualifyingPopulation );

        m_pCurrentAction = GetAction( incidence_value );

        // If p_action = nullptr, then a threshold was not achieved and nothing will be broadcasted.
        if( m_pCurrentAction != nullptr )
        {
            int num_distributed = 0;
            for( INodeEventContext* p_node : nodes )
            {
                num_distributed += p_node->VisitIndividuals( this, -1 );
            }
            std::stringstream ss;
            ss << "UpdateNodes() broadcasted '" << m_pCurrentAction->GetEventToBroadcast().ToString() << "' to " << num_distributed << " individuals\n";
            LOG_INFO( ss.str().c_str() );
        }
    }

    float Responder::CalculateIndidence( uint32_t numIncidences, uint32_t qualifyingPopulation ) const
    {
        float value = float( numIncidences );
        if( m_ThresholdType == ThresholdType::PERCENTAGE )
        {
            value = value / float(qualifyingPopulation);
        }
        return value;
    }

    Action* Responder::GetAction( float value )
    {
        Action* p_action = nullptr;
        for( int i = 0; i < m_ActionList.Size(); ++i )
        {
            if( value >= m_ActionList[ i ]->GetThreshold() )
            {
                p_action = m_ActionList[ i ];
                break;
            }
        }
        return p_action;
    }

    // ------------------------------------------------------------------------
    // --- IncidenceCounter
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY( IncidenceCounter )
    END_QUERY_INTERFACE_BODY( IncidenceCounter )

    IncidenceCounter::IncidenceCounter()
        : JsonConfigurable()
        , m_Count(0)
        , m_CountEventsForNumTimeSteps(1)
        , m_NumTimeStepsCounted(-1)
        , m_IsDoneCounting(false)
        , m_DemographicRestrictions()
        , m_TriggerConditionList()
        , m_NodePropertyRestrictions()
    {
        m_NumTimeStepsCounted = -1;
    }

    IncidenceCounter::~IncidenceCounter()
    {
    }

    bool IncidenceCounter::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Count_Events_For_Num_Timesteps", &m_CountEventsForNumTimeSteps, ICE_Count_Events_For_Num_Timesteps_DESC_TEXT, 1, INT_MAX, 1 );

        initConfigTypeMap( "Trigger_Condition_List", &m_TriggerConditionList, ICE_Trigger_Condition_List_DESC_TEXT );

        initConfigComplexType( "Node_Property_Restrictions", &m_NodePropertyRestrictions, ICE_Node_Property_Restriction_DESC_TEXT );

        m_DemographicRestrictions.ConfigureRestrictions( this, inputJson );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_DemographicRestrictions.CheckConfiguration();
        }
        return ret;
    }


    bool IncidenceCounter::notifyOnEvent( IIndividualHumanEventContext *context,
                                          const EventTrigger& trigger )
    {
        if( m_NodePropertyRestrictions.Qualifies( context->GetNodeEventContext()->GetNodeContext()->GetNodeProperties() ) &&
            m_DemographicRestrictions.IsQualified( context ) &&
            !IsDoneCounting() )
        {
            float demographic_coverage = m_DemographicRestrictions.GetDemographicCoverage();
            if( (demographic_coverage > 0.0) )
            {
                bool count_event = true;
                // don't draw random number if coverage equals 1.
                if( demographic_coverage < 1.0 )
                {
                    count_event = (randgen->e() <= demographic_coverage);
                }
                if( count_event )
                {
                    ++m_Count;
                }
            }
        }

        return true;
    }

    uint32_t IncidenceCounter::GetCount() const
    {
        return m_Count;
    }

    // I can't seem to make this const due to the use of individual_visit_function
    uint32_t IncidenceCounter::GetCountOfQualifyingPopulation( const std::vector<INodeEventContext*>& rNodes )
    {
        uint32_t pop = 0;

        INodeEventContext::individual_visit_function_t fn =
            [ this, &pop ]( IIndividualHumanEventContext *phec )
        {
            if( m_NodePropertyRestrictions.Qualifies( phec->GetNodeEventContext()->GetNodeContext()->GetNodeProperties() ) &&
                m_DemographicRestrictions.IsQualified( phec ) )
            {
                ++pop;
            }
        };

        for( INodeEventContext* p_nec : rNodes )
        {
            p_nec->VisitIndividuals( fn );
        }

        return pop;
    }

    void IncidenceCounter::StartCounting()
    {
        m_NumTimeStepsCounted = 0;
        m_IsDoneCounting = false;
        m_Count = 0;
    }

    void IncidenceCounter::Update( float dt )
    {
        ++m_NumTimeStepsCounted;
        if( m_NumTimeStepsCounted >= m_CountEventsForNumTimeSteps )
        {
            m_IsDoneCounting = true;
        }
    }

    bool IncidenceCounter::IsDoneCounting() const
    {
        return m_IsDoneCounting;
    }

    void IncidenceCounter::RegisterForEvents( INodeEventContext* pNEC )
    {
        INodeTriggeredInterventionConsumer* pNTIC = GetNodeTriggeredConsumer( pNEC );

        for( auto& trigger : m_TriggerConditionList )
        {
            pNTIC->RegisterNodeEventObserver( this, trigger );
        }
    }

    void IncidenceCounter::UnregisterForEvents( INodeEventContext* pNEC )
    {
        INodeTriggeredInterventionConsumer* pNTIC = GetNodeTriggeredConsumer( pNEC );

        for( auto& trigger : m_TriggerConditionList )
        {
            pNTIC->UnregisterNodeEventObserver( this, trigger );
        }
    }

    INodeTriggeredInterventionConsumer* IncidenceCounter::GetNodeTriggeredConsumer( INodeEventContext* pNEC )
    {
        release_assert( pNEC );
        INodeTriggeredInterventionConsumer* pNTIC = nullptr;
        if( pNEC->QueryInterface( GET_IID( INodeTriggeredInterventionConsumer ), (void**)&pNTIC ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNEC", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
        }
        release_assert( pNTIC );

        return pNTIC;
    }

    int32_t IncidenceCounter::GetCountEventsForNumTimeSteps() const
    {
        return m_CountEventsForNumTimeSteps;
    }

    // ------------------------------------------------------------------------
    // --- IncidenceEventCoordinator
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED( IncidenceEventCoordinator )
    IMPL_QUERY_INTERFACE2( IncidenceEventCoordinator, IEventCoordinator, IConfigurable )

    IncidenceEventCoordinator::IncidenceEventCoordinator()
        : JsonConfigurable()
        , m_Parent( nullptr )
        , m_CachedNodes()
        , m_IsExpired( false )
        , m_HasBeenDistributed( false )
        , m_NumReps( -1 )
        , m_NumTimestepsBetweenReps(1)
        , m_NumTimestepsInRep(0)
        , m_IncidenceCounter()
        , m_Responder()
    {
        // Start at zero in constructor so it gets set to 1 on first call to Update()
        m_NumTimestepsInRep = 0;
    }

    IncidenceEventCoordinator::~IncidenceEventCoordinator()
    {
    }


    QuickBuilder IncidenceEventCoordinator::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    bool IncidenceEventCoordinator::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Number_Repetitions", &m_NumReps, ICE_Number_Repetitions_DESC_TEXT, -1, 1000, 1 );
        initConfigTypeMap( "Timesteps_Between_Repetitions", &m_NumTimestepsBetweenReps, ICE_Timesteps_Between_Repetitions_DESC_TEXT, -1, 10000, -1 );
        initConfigTypeMap( "Incidence_Counter", &m_IncidenceCounter, ICE_Incidence_Counter_DESC_TEXT );
        initConfigTypeMap( "Responder", &m_Responder, ICE_Responder_DESC_TEXT );

        bool retValue = JsonConfigurable::Configure( inputJson );

        if( retValue && !JsonConfigurable::_dryrun )
        {
            if( m_NumTimestepsBetweenReps < m_IncidenceCounter.GetCountEventsForNumTimeSteps() )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "Timesteps_Between_Repetitions", m_NumTimestepsBetweenReps,
                                                        "Count_Events_For_Num_Timesteps", m_IncidenceCounter.GetCountEventsForNumTimeSteps(),
                                                        "'Timesteps_Between_Repetitions' must be >= 'Count_Events_For_Num_Timesteps'" );
            }
        }

        return retValue;
    }

    void IncidenceEventCoordinator::SetContextTo( ISimulationEventContext *isec )
    {
        m_Parent = isec;
    }

    void IncidenceEventCoordinator::AddNode( const suids::suid& node_suid )
    {
        INodeEventContext* pNEC = m_Parent->GetNodeEventContext( node_suid );
        m_CachedNodes.push_back( pNEC );

        m_IncidenceCounter.RegisterForEvents( pNEC );
    }

    // Update() is called for all the coordinators before calling UpdateNodes()
    void IncidenceEventCoordinator::Update( float dt )
    {
        // -----------------------------------------------------------------------
        // --- Update the counter of time steps in this rep.
        // --- UpdateNodes() will be called before IsFinished() is checked and the
        // --- coordinator deleted.
        // -----------------------------------------------------------------------
        ++m_NumTimestepsInRep;
    }

    void IncidenceEventCoordinator::UpdateNodes( float dt )
    {
        m_IncidenceCounter.Update( dt );

        if( m_IncidenceCounter.IsDoneCounting() && !m_HasBeenDistributed )
        {
            uint32_t num_incidences = m_IncidenceCounter.GetCount();
            uint32_t num_qualifying_pop = m_IncidenceCounter.GetCountOfQualifyingPopulation( m_CachedNodes );
            m_Responder.Distribute( m_CachedNodes, num_incidences, num_qualifying_pop );
            m_HasBeenDistributed = true;
        }

        if( m_NumTimestepsInRep > m_NumTimestepsBetweenReps )
        {
            // Start at 1 because we are setting this after Update()
            m_NumTimestepsInRep = 1;

            m_IncidenceCounter.StartCounting();
            m_HasBeenDistributed = false;

            if( m_NumReps > -1 )
            {
                --m_NumReps;
                m_IsExpired = (m_NumReps <= 0 );
            }
        }
    }

    bool IncidenceEventCoordinator::IsFinished()
    {
        if( m_IsExpired )
        {
            for( auto pNEC : m_CachedNodes )
            {
                m_IncidenceCounter.UnregisterForEvents( pNEC );
            }
        }
        return m_IsExpired;
    }
}

