
#include "stdafx.h"

#include "BaseEventReport.h"

#include "report_params.rc"
#include "ISupports.h"
#include "NodeEventContext.h"
#include "EventTrigger.h"
#include "SimulationEventContext.h"

// DON'T COMMIT THIS CHANGE BELOW
SETUP_LOGGING( "BaseEventReport" )

namespace Kernel
{
    BaseEventReport::BaseEventReport( const std::string& rReportName, bool useHumanMinMaxAge, bool useHumanOther )
        : IReport()
        , reportName(rReportName)
        , report_filter( nullptr, "", false, useHumanMinMaxAge, useHumanOther )
        , eventTriggerList()
        , events_registered(false)
        , events_unregistered(false)
        , nodeEventContextList()
    {
    }

    BaseEventReport::~BaseEventReport()
    {
    }

    // ---------------------
    // --- JsonConfigurable
    // ---------------------

    bool BaseEventReport::Configure( const Configuration* inputJson )
    {
        report_filter.ConfigureParameters( *this, inputJson );
        ConfigureEvents( inputJson );

        bool retValue = JsonConfigurable::Configure( inputJson );
        if( retValue && !JsonConfigurable::_dryrun )
        {
            report_filter.CheckParameters( inputJson );
            CheckConfigurationEvents();
        }
        return retValue ;
    }

    void BaseEventReport::ConfigureEvents( const Configuration* inputJson )
    {
        initConfigTypeMap( "Event_Trigger_List", &eventTriggerList, Report_Event_Trigger_List_DESC_TEXT );
    }

    void BaseEventReport::CheckConfigurationEvents()
    {
        if( eventTriggerList.size() == 0 )
        {
            std::stringstream ss;
            ss << "'Event_Trigger_List' cannot be empty in report " << GetReportName();
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
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
        report_filter.Initialize();
    }

    void BaseEventReport::CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& nodeIds_demographics)
    {
        report_filter.CheckForValidNodeIDs( GetReportName(), nodeIds_demographics );
    }

    void BaseEventReport::UpdateEventRegistration( float currentTime,
                                                   float dt,
                                                   std::vector<INodeEventContext*>& rNodeEventContextList,
                                                   ISimulationEventContext* pSimEventContext )
    {
        bool is_valid_time = report_filter.IsValidTime( pSimEventContext->GetSimulationTime() );
        bool register_now = false ;
        bool unregister_now = false ;
        if( !events_registered && is_valid_time )
        {
            // register events during this time step
            register_now = true ;

            // ------------------------------------------------------------------------------
            // --- set this member variable to true here versus RegisterEvents() because
            // --- we want the report on this core to know that it is time to collect data
            // --- even if the report does not have any nodes on this core.  This will ensure
            // --- that the core communicates with the other cores.
            // ------------------------------------------------------------------------------
            events_registered = true;
        }
        else if( events_registered && !events_unregistered && !is_valid_time )
        {
            // unregister the events during this time step
            unregister_now = true ;

            // ----------------------------------------------------------------------------
            // --- This member variable is set here like events_registered so that it will
            // --- be set even if the report does not have any nodes on this core.
            // ----------------------------------------------------------------------------
            events_unregistered = true;
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
                if( report_filter.IsValidNode( p_nec ) )
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
        return report_filter.GetStartDay();
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
    }

    void BaseEventReport::UnregisterEvents( INodeEventContext* pNEC )
    {
        IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();

        for( auto trigger : eventTriggerList )
        {
            LOG_DEBUG_F( "BaseEventReport is unregistering to listen to event %s\n", trigger.c_str() );
            broadcaster->UnregisterObserver( this, trigger );
        }
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
        return report_filter.GetNewReportName( reportName );
    }
}
