/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "DelayEventCoordinator.h"
#include "DistributionFactory.h"

SETUP_LOGGING( "DelayEventCoordinator" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED( DelayEventCoordinator )
    IMPL_QUERY_INTERFACE2( DelayEventCoordinator, IEventCoordinator, IConfigurable )

    DelayEventCoordinator::DelayEventCoordinator()
        : TriggeredEventCoordinator()
        , remaining_delay_days( 0.0 )
        , delay_distribution( nullptr )
    {
        remaining_delay_days.handle = std::bind( &DelayEventCoordinator::Callback, this, std::placeholders::_1 );   //Callback is called when counter expires

        initSimTypes( 1, "*" );
    }

    DelayEventCoordinator::~DelayEventCoordinator()
    {
        delete delay_distribution;
    }

    void DelayEventCoordinator::Callback( float dt )
    {
        LOG_DEBUG( "Counter expired, broadcasting completion event." );
        m_IsActive = false;
        BroadcastCompletionEvent();
    }

    bool DelayEventCoordinator::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Start_Trigger_Condition_List", &m_StartTriggerConditionList, TEC_Start_Trigger_Condition_List_DESC_TEXT );
        initConfigTypeMap( "Stop_Trigger_Condition_List", &m_StopTriggerConditionList, TEC_Stop_Trigger_Condition_List_DESC_TEXT );
        initConfigTypeMap( "Delay_Complete_Event", &m_CompletionEvent, DEC_Completion_Event_DESC_TEXT );
        initConfigTypeMap( "Coordinator_Name", &m_CoordinatorName, TEC_Coordinator_Name_DESC_TEXT, "DelayEventCoordinator" );
        initConfigTypeMap( "Duration", &m_Duration, TEC_Duration_DESC_TEXT, -1.0f, FLT_MAX, -1.0f );

        DistributionFunction::Enum delay_function( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Delay_Period_Distribution", delay_function, inputJson, MetadataDescriptor::Enum( "Delay_Period", DI_Delay_Distribution_DESC_TEXT, MDD_ENUM_ARGS( DistributionFunction ) ) );      
        delay_distribution = DistributionFactory::CreateDistribution( this, delay_function, "Delay_Period", inputJson );

        bool retValue = JsonConfigurable::Configure( inputJson );

        if( retValue && !JsonConfigurable::_dryrun )
        {
            CheckConfigTriggers( inputJson );

            if( m_CompletionEvent.IsUninitialized() )
            {
                std::stringstream ss;
                ss << "'Delay_Complete_Event', for coordinator '" << m_CoordinatorName << "', must be defined and it cannot be empty.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        return retValue;
    }

    void DelayEventCoordinator::UpdateNodes( float dt )
    {
        if( !m_IsActive ) return;
        remaining_delay_days.Decrement( dt );
    }

    void DelayEventCoordinator::Update( float dt )
    {
        if( m_Duration != -1.0f )
        {
            if( m_Duration <= 0 )
            {
                m_DurationExpired = true;
            }
            else
            {
                m_Duration -= dt;
            }
        }        
    }

    bool DelayEventCoordinator::notifyOnEvent( IEventCoordinatorEventContext *pEntity, const EventTriggerCoordinator& trigger )
    {
        auto it_start = find( m_StartTriggerConditionList.begin(), m_StartTriggerConditionList.end(), trigger );
        if( it_start != m_StartTriggerConditionList.end() )
        {
            LOG_INFO_F( "%s: notifyOnEvent received start: %s\n", m_CoordinatorName.c_str(), trigger.ToString().c_str() );

            // ---------------------------------------------------------------------
            // --- Not sure this is the best solution, but always using the RNG from
            // --- the first node should at least be consistent.
            // ---------------------------------------------------------------------
            release_assert( cached_nodes.size() > 0 );
            RANDOMBASE* p_rng = cached_nodes[0]->GetRng();

            m_IsActive = true;
            remaining_delay_days = delay_distribution->Calculate( p_rng );
        }
        else
        {
            LOG_INFO_F( "%s: notifyOnEvent received stop: %s\n", m_CoordinatorName.c_str(), trigger.ToString().c_str() );
            m_IsActive = false;
        }
        return true;
    }
}