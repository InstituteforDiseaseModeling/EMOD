/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "ISupports.h"
#include "BaseEventReport.h"
#include "NodeEventContext.h"
#include "EventTrigger.h"

// DON'T COMMIT THIS CHANGE BELOW
SETUP_LOGGING( "BaseEventReport" )

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
        initConfigTypeMap( "Duration_Days",  &durationDays, Duration_Days_DESC_TEXT, 0.0f, FLT_MAX, FLT_MAX );
        if( inputJson->Exist("Report_Description") )
        {
            initConfigTypeMap( "Report_Description", &reportDescription, Report_Description_DESC_TEXT );
        }
        if( inputJson->Exist("Nodeset_Config") )
        {
            initConfigComplexType( "Nodeset_Config", &nodesetConfig, Nodeset_Config_DESC_TEXT );
        }
        else
        {
            pNodeSet = new NodeSetAll();
        }

        if( inputJson->Exist( "Event_Trigger_List" ) )
        {
            initConfigTypeMap( "Event_Trigger_List", &eventTriggerList, Event_Trigger_List_DESC_TEXT );
        }

        bool retValue = JsonConfigurable::Configure( inputJson );
        if( eventTriggerList.size() == 0 )
        {
            eventTriggerList = EventTriggerFactory::GetInstance()->GetAllEventTriggers();
        }

        if( retValue && (pNodeSet == nullptr) )
        {
            auto tmp = Configuration::CopyFromElement( nodesetConfig._json, inputJson->GetDataLocation() );
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

    void BaseEventReport::CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& nodeIds_demographics)
    {
        std::vector<ExternalNodeId_t> nodes_missing_in_demographics = pNodeSet->IsSubset(nodeIds_demographics);
        if (!nodes_missing_in_demographics.empty())
        {
            std::stringstream nodes_missing_in_demographics_str;
            std::copy(nodes_missing_in_demographics.begin(), nodes_missing_in_demographics.end(), ostream_iterator<int>(nodes_missing_in_demographics_str, " "));  // list of missing nodes

            std::stringstream error_msg;
            error_msg <<"Found NodeIDs in " << GetReportName().c_str() << " that are missing in demographics: ";
            error_msg << nodes_missing_in_demographics_str.str() << ". Only nodes configured in demographics can be used in a report.";
            throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, error_msg.str().c_str());
        }
    }

    void BaseEventReport::UpdateEventRegistration( float currentTime,
                                                   float dt,
                                                   std::vector<INodeEventContext*>& rNodeEventContextList,
                                                   ISimulationEventContext* pSimEventContext )
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

    void BaseEventReport::LogIndividualData( IIndividualHuman* individual )
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

    const std::vector< EventTrigger >&
    BaseEventReport::GetEventTriggerList() const
    {
        return eventTriggerList;
    }

    // -----------------
    // --- Other Methods
    // -----------------

    void BaseEventReport::RegisterEvents( INodeEventContext* pNEC )
    {
        IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();

        for( auto trigger : eventTriggerList )
        {
            LOG_DEBUG_F( "BaseEventReport is registering to listen to event %s\n", trigger.c_str() );
            broadcaster->RegisterObserver( this, trigger );
        }
        nodeEventContextList.push_back( pNEC );
        events_registered = true ;
    }

    void BaseEventReport::UnregisterEvents( INodeEventContext* pNEC )
    {
        IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();

        for( auto trigger : eventTriggerList )
        {
            LOG_DEBUG_F( "BaseEventReport is unregistering to listen to event %s\n", trigger.c_str() );
            broadcaster->UnregisterObserver( this, trigger );
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
