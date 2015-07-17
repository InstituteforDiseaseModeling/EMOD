/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "ISupports.h"
#include "BaseEventReport.h"
#include "NodeEventContext.h"
#include "Types.h"

static const char * _module = "BaseEventReport";

namespace Kernel
{
    BaseEventReport::BaseEventReport( const std::string& rReportName )
        : IReport()
        , reportName(rReportName)
        , startDay(0.0f)
        , durationDays(0.0f)
        , reportDescription()
        , pNodeSet(nullptr)
        , nodesetConfig()
        , eventTriggerList()
        , events_registered(false)
        , events_unregistered(false)
        , nodeEventContextList()
    {
    }

    BaseEventReport::~BaseEventReport()
    {
        delete pNodeSet ;
        pNodeSet = nullptr ;
    }

    // ---------------------
    // --- JsonConfigurable
    // ---------------------

    bool BaseEventReport::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Start_Day",      &startDay,     Start_Day_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "Duration_Days",  &durationDays, Event_Report_Duration_Days_DESC_TEXT, 0.0f, FLT_MAX, FLT_MAX );
        if( inputJson->Exist("Report_Description") )
        {
            initConfigTypeMap( "Report_Description", &reportDescription, Event_Report_Report_Description_DESC_TEXT );
        }
        if( inputJson->Exist("Nodeset_Config") )
        {
            initConfigComplexType( "Nodeset_Config", &nodesetConfig, Nodeset_Config_DESC_TEXT );
        }
        else
        {
            pNodeSet = new NodeSetAll();
        }

        // -------------------------------------------------------------------------------
        // --- Listed_Events - This code is somewhat duplicated from SimulationConfig.
        // --- We don't want to use SimulationConfig here because the custom reports would
        // --- need to include SimulationConfig and it brings along too much baggage.
        // -------------------------------------------------------------------------------
        std::vector<std::string> listed_events_list = GET_CONFIG_VECTOR_STRING( EnvPtr->Config, "Listed_Events" );
        JsonConfigurable::tDynamicStringSet listed_events;
        for( auto event_str : listed_events_list )
        {
            listed_events.insert( event_str );
        }
        // Add Built-in Events
        for( int i = 0 ; i < IndividualEventTriggerType::pairs::count()-2 ; i++ )
        {
            auto trigger = IndividualEventTriggerType::pairs::lookup_key( i );
            if( trigger != nullptr )
            {
                listed_events.insert( trigger );
            }
        }


        initConfigTypeMap( "Event_Trigger_List", &eventTriggerList, Event_Report_Event_Trigger_List_DESC_TEXT, "<configuration>:Listed_Events.*", listed_events );
        bool retValue = JsonConfigurable::Configure( inputJson );
        if( eventTriggerList.size() == 0 )
        {
            for( auto &trigger: listed_events )
            {
                eventTriggerList.push_back( trigger );
            }
        }

        if( retValue && (pNodeSet == nullptr) )
        {
            auto tmp = Configuration::CopyFromElement( nodesetConfig._json );
            pNodeSet = NodeSetFactory::CreateInstance( tmp );
            delete tmp;
        }

        return retValue ;
    }

    // ------------
    // --- IReport
    // ------------

    std::string BaseEventReport::GetReportName() const
    {
        return reportName ;
    }

    void BaseEventReport::Initialize( unsigned int nrmSize )
    {
    }

    void BaseEventReport::UpdateEventRegistration( float currentTime, 
                                                   float dt, 
                                                   std::vector<INodeEventContext*>& rNodeEventContextList )
    {
        bool register_now = false ;
        bool unregister_now = false ;
        if( !events_registered && (currentTime >= startDay) )
        {
            register_now = true ;
        }
        else if( events_registered && !events_unregistered && (currentTime >= (startDay + durationDays)) )
        {
            unregister_now = true ;
        }
        // --------------------------------------------------------
        // --- if the events have been registered and unregistered, 
        // --- then we are NOT going to register them again.
        // --------------------------------------------------------

        // try to only loop over the nodes if we are actively registering or unregistering
        if( register_now || unregister_now )
        {
            for( auto p_nec : rNodeEventContextList )
            {
                if( pNodeSet->Contains( p_nec ) )
                {
                    if( register_now )
                    {
                        RegisterEvents( p_nec );
                    }
                    else if( unregister_now )
                    {
                        UnregisterEvents( p_nec );
                    }
                }
            }
        }
    }

    void BaseEventReport::BeginTimestep()
    {
    }

    bool BaseEventReport::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return false ;
    }

    void BaseEventReport::LogIndividualData( IndividualHuman * individual )
    {
    }

    void BaseEventReport::LogNodeData( INodeContext * pNC )
    {
    }

    void BaseEventReport::EndTimestep( float currentTime, float dt )
    {
    }

    void BaseEventReport::Reduce()
    {
        UnregisterAllNodes();
    }

    void BaseEventReport::Finalize()
    {
    }

    // -----------------------------
    // --- IIndividualEventObserver
    // -----------------------------

#if USE_JSON_SERIALIZATION
    // For JSON serialization
    void BaseEventReport::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
    }

    void BaseEventReport::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
    }
#endif

    float
    BaseEventReport::GetStartDay()
    const
    {
        return startDay;
    }

    float
    BaseEventReport::GetDurationDays()
    const
    {
        return durationDays;
    }

    const std::vector< std::string >&
    BaseEventReport::GetEventTriggerList() const
    {
        return eventTriggerList;
    }

    // -----------------
    // --- Other Methods
    // -----------------

    void BaseEventReport::RegisterEvents( INodeEventContext* pNEC )
    {
        INodeTriggeredInterventionConsumer* pNTIC = GetNodeTriggeredConsumer( pNEC );

        for( auto trigger : eventTriggerList )
        {
            LOG_DEBUG_F( "BaseEventReport is registering to listen to event %s\n", trigger.c_str() );
            pNTIC->RegisterNodeEventObserverByString( this, trigger );
        }
        nodeEventContextList.push_back( pNEC );
        events_registered = true ;
    }

    void BaseEventReport::UnregisterEvents( INodeEventContext* pNEC )
    {
        INodeTriggeredInterventionConsumer* pNTIC = GetNodeTriggeredConsumer( pNEC );

        for( auto trigger : eventTriggerList )
        {
            LOG_DEBUG_F( "BaseEventReport is unregistering to listen to event %s\n", trigger.c_str() );
            pNTIC->UnregisterNodeEventObserverByString( this, trigger );
        }
        events_unregistered = true ;
    }

    void BaseEventReport::UnregisterAllNodes()
    {
        if( events_registered && !events_unregistered )
        {
            for( auto p_nec : nodeEventContextList )
            {
                UnregisterEvents( p_nec );
            }
        }
    }

    INodeTriggeredInterventionConsumer* 
        BaseEventReport::GetNodeTriggeredConsumer( INodeEventContext* pNEC )
    {
        release_assert( pNEC );
        INodeTriggeredInterventionConsumer* pNTIC = nullptr;
        if( pNEC->QueryInterface( GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNEC", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
        }
        release_assert( pNTIC );

        return pNTIC ;
    }

    bool BaseEventReport::HaveRegisteredAllEvents() const
    {
        return events_registered ;
    }

    bool BaseEventReport::HaveUnregisteredAllEvents() const
    {
        return events_unregistered ;
    }
    
    Kernel::INodeEventContext* BaseEventReport::GetFirstINodeEventContext()
    {
        INodeEventContext* p_nec = nullptr ;
        if( nodeEventContextList.size() > 0 )
        {
            p_nec = nodeEventContextList[0] ;
        }
        return p_nec ;
    }

    std::string BaseEventReport::GetBaseOutputFilename() const
    {
        std::string output_fn = reportName ;
        if( !reportDescription.empty() )
        {
            output_fn += "_" + reportDescription ;
        }
        return output_fn ;
    }
}
