/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReportEventRecorderCoordinator.h"
#include "Log.h"
#include "Exceptions.h"
#include "IdmDateTime.h"
#include "EventCoordinator.h"
#include "SimulationEventContext.h"
#include "EventTriggerCoordinator.h"

SETUP_LOGGING( "ReportEventRecorderCoordinator" )

// These need to be after SETUP_LOGGING so that the LOG messages in the
// templates don't make GCC complain.
#include "BaseTextReportEventsTemplate.h"
#include "BaseReportEventRecorderTemplate.h"

namespace Kernel
{
    template std::string BaseReportEventRecorder< ICoordinatorEventBroadcaster,
                                                  ICoordinatorEventObserver,
                                                  IEventCoordinatorEventContext,
                                                  EventTriggerCoordinator,
                                                  EventTriggerCoordinatorFactory>::GetEnableParameterName();

    template void BaseTextReportEventsTemplate< ICoordinatorEventBroadcaster,
                                                ICoordinatorEventObserver,
                                                IEventCoordinatorEventContext,
                                                EventTriggerCoordinator >::Reduce();

    const std::string ReportEventRecorderCoordinator::ENABLE_PARAMETER_NAME   = "Report_Coordinator_Event_Recorder";
    const std::string ReportEventRecorderCoordinator::EVENTS_LIST_NAME        = "Report_Coordinator_Event_Recorder_Events";
    const std::string ReportEventRecorderCoordinator::EVENTS_LIST_DESC        =  Report_Coordinator_Event_Recorder_Events_DESC_TEXT;
    const std::string ReportEventRecorderCoordinator::IGNORE_EVENTS_LIST_NAME = "Report_Coordinator_Event_Recorder_Ignore_Events_In_List";
    const std::string ReportEventRecorderCoordinator::IGNORE_EVENTS_LIST_DESC =  Report_Coordinator_Event_Recorder_Ignore_Events_In_List_DESC_TEXT;

    GET_SCHEMA_STATIC_WRAPPER_IMPL( ReportEventRecorderCoordinator, ReportEventRecorderCoordinator )

    IReport* ReportEventRecorderCoordinator::CreateReport()
    {
        return new ReportEventRecorderCoordinator();
    }

    ReportEventRecorderCoordinator::ReportEventRecorderCoordinator()
        : BaseReportEventRecorder( "ReportCoordinatorEventRecorder.csv" )
    {
    }

    ReportEventRecorderCoordinator::ReportEventRecorderCoordinator( const std::string& rReportName )
        : BaseReportEventRecorder( rReportName )
    {
    }

    ReportEventRecorderCoordinator::~ReportEventRecorderCoordinator()
    {
    }

    void ReportEventRecorderCoordinator::UpdateEventRegistration( float currentTime,
                                                                  float dt,
                                                                  std::vector<INodeEventContext*>& rNodeEventContextList,
                                                                  ISimulationEventContext* pSimEventContext )
    {
        if( !is_registered )
        {
            for( auto trigger : eventTriggerList )
            {
                pSimEventContext->GetCoordinatorEventBroadcaster()->RegisterObserver( this, trigger );
            }
            is_registered = true;
        }
    }

    std::string ReportEventRecorderCoordinator::GetHeader() const
    {
        std::stringstream header;
        header << BaseReportEventRecorder::GetHeader()
               << "," << "Coordinator_Name"
               << "," << "Event_Name";

        return header.str();
    }

    std::string ReportEventRecorderCoordinator::GetOtherData( IEventCoordinatorEventContext *pEntity,
                                                              const EventTriggerCoordinator& trigger )
    {
        std::stringstream ss;

        ss << "," << pEntity->GetName()
           << "," << trigger.ToString();

        return ss.str();
    }

    float ReportEventRecorderCoordinator::GetTime( IEventCoordinatorEventContext* pEntity ) const
    {
        return pEntity->GetTime().time;
    }
}
