/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "NodePropertyValueChanger.h"
#include "Contexts.h"
#include "Debug.h" // for release_assert
#include "RANDOM.h"
#include "Common.h"             // for INFINITE_TIME
#include "MathFunctions.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)

SETUP_LOGGING( "NodePropertyValueChanger" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED( NodePropertyValueChanger )

    BEGIN_QUERY_INTERFACE_BODY( NodePropertyValueChanger )
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE( IBaseIntervention )
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_ISUPPORTS_VIA( INodeDistributableIntervention )
    END_QUERY_INTERFACE_BODY( NodePropertyValueChanger )

    NodePropertyValueChanger::NodePropertyValueChanger()
        : BaseNodeIntervention()
        , m_TargetKeyValue()
        , probability(1.0)
        , revert(0.0f)
        , max_duration(0.0f)
        , action_timer(0.0f)
        , reversion_timer(0.0f)
    {
    }

    NodePropertyValueChanger::NodePropertyValueChanger( const NodePropertyValueChanger& rThat )
        : BaseNodeIntervention( rThat )
        , m_TargetKeyValue( rThat.m_TargetKeyValue )
        , probability( rThat.probability )
        , revert( rThat.revert )
        , max_duration( rThat.max_duration )
        , action_timer( 0.0f )
        , reversion_timer( rThat.reversion_timer )
    {
        SetActionTimer( this );
    }

    NodePropertyValueChanger::~NodePropertyValueChanger()
    {
    }

    bool NodePropertyValueChanger::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Target_NP_Key_Value", &m_TargetKeyValue, NPC_Target_NP_Key_Value_DESC_TEXT );
        initConfigTypeMap("Daily_Probability",   &probability,      NPC_Daily_Probability_DESC_TEXT, 0.0f, 1.0f );
        initConfigTypeMap("Maximum_Duration",    &max_duration,     NPC_Maximum_Duration_DESC_TEXT, -1.0f, FLT_MAX, FLT_MAX);
        initConfigTypeMap("Revert",              &revert,           NPC_Revert_DESC_TEXT,            0.0f, 10000.0f, 0.0f );

        bool ret = BaseNodeIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            SetActionTimer( this );
            if( !m_TargetKeyValue.IsValid() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "You must specify 'Target_NP_Key_Value'" );
            }
        }
        return ret;
    }

    void NodePropertyValueChanger::SetActionTimer( NodePropertyValueChanger* npvc )
    {
        if( npvc->probability < 1.0 )
        {
            npvc->action_timer = Probability::getInstance()->fromDistribution( DistributionFunction::EXPONENTIAL_DURATION, npvc->probability, 0, 0, 0 );
            if( npvc->action_timer > npvc->max_duration )
            {
                npvc->action_timer = FLT_MAX;
            }
            LOG_DEBUG_F( "Time until property change occurs = %f\n", npvc->action_timer );
        }
    }

    bool NodePropertyValueChanger::Distribute( INodeEventContext *pNodeEventContext, IEventCoordinator2 *pEC )
    {
        return BaseNodeIntervention::Distribute( pNodeEventContext, pEC );
    }

    void NodePropertyValueChanger::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        INodeContext* p_node_context = parent->GetNodeContext();

        NPKeyValue current_prop_value;
        if( reversion_timer > 0 )
        {
            reversion_timer -= dt;
            if( reversion_timer <= 0 )
            {
                probability = 1.0;
            }
        }

        if( probability == 1.0 || action_timer < 0 )
        {
            if( revert )
            {
                current_prop_value = p_node_context->GetNodeProperties().Get( m_TargetKeyValue.GetKey<NPKey>() );
            }
            p_node_context->GetNodeProperties().Set( m_TargetKeyValue );

            //broadcast that the individual changed properties
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != parent->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
            broadcaster->TriggerNodeEventObservers( nullptr, EventTrigger::NodePropertyChange );


            if( revert )
            {
                m_TargetKeyValue = current_prop_value;
                probability = 0.0; // keep it simple for now, reversion is guaranteed
                reversion_timer = revert;
                action_timer= FLT_MAX;
                LOG_DEBUG_F( "Initializing reversion timer to %f\n", reversion_timer );
                revert = 0; // no more reversion from reversion
            }
            else
            {
                expired = true;
            }
        }
        action_timer -= dt;
    }

    int NodePropertyValueChanger::AddRef()
    {
        return BaseNodeIntervention::AddRef();
    }

    int NodePropertyValueChanger::Release()
    {
        return BaseNodeIntervention::Release();
    }
}
