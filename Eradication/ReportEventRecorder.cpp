/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <string>
#include "ReportEventRecorder.h"
#include "Log.h"
#include "Exceptions.h"
#include "NodeEventContext.h"
#include "IndividualEventContext.h"
#include "Contexts.h"
#include "FileSystem.h"
#include "SimulationEnums.h"
#include "ISimulation.h"
#include "EventTrigger.h"

SETUP_LOGGING( "ReportEventRecorder" )

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(ReportEventRecorder,ReportEventRecorder)

    IReport* ReportEventRecorder::CreateReport()
    {
        return new ReportEventRecorder();
    }

    ReportEventRecorder::ReportEventRecorder()
        : BaseTextReportEvents("ReportEventRecorder.csv")
        , ignore_events_in_list(false)
    {
    }

    ReportEventRecorder::~ReportEventRecorder()
    {
    }

    bool ReportEventRecorder::Configure( const Configuration * inputJson )
    {
        std::vector<EventTrigger> tmp_event_trigger_list ;

        initConfigTypeMap( "Report_Event_Recorder_Ignore_Events_In_List", &ignore_events_in_list, Report_Event_Recorder_Ignore_Events_In_List_DESC_TEXT, false, "Report_Event_Recorder" );
        initConfigTypeMap( "Report_Event_Recorder_Events", &tmp_event_trigger_list, Report_Event_Recorder_Events_DESC_TEXT, "Report_Event_Recorder" );

        if( inputJson && inputJson->Exist("Report_Event_Recorder_Individual_Properties" ) || JsonConfigurable::_dryrun )
        {
            properties_to_report.value_source = IPKey::GetConstrainedStringConstraintKey(); 
            // xpath-y way of saying that the possible values for prop restrictions comes from demographics file IP's.
            initConfigTypeMap("Report_Event_Recorder_Individual_Properties", &properties_to_report, Property_Restriction_DESC_TEXT, "Intervention_Config.*.iv_type", "IndividualTargeted" );
        }

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            if( !ignore_events_in_list && tmp_event_trigger_list.empty() )
            {
                LOG_WARN( "No data will be recorded.  The Report_Event_Recorder_Events list is empty and Report_Event_Recorder_Ignore_Events_In_List is false.\n" );
            }
            else
            {
                // This logic goes through all possible events.  It checks to see if that event
                // is in the listen-to-these event_list provided by the user. But that list can be a 
                // whitelist or blacklist. If using whitelist AND event-requested is in master THEN listen.
                // else if using blacklist AND if event-(de)requested is not in master THEN listen.

                std::vector<EventTrigger> all_trigger_list = EventTriggerFactory::GetInstance()->GetAllEventTriggers();
                for( auto trigger : all_trigger_list )
                {
                    bool in_event_list = std::find( tmp_event_trigger_list.begin(), 
                                                    tmp_event_trigger_list.end(), trigger ) != tmp_event_trigger_list.end() ;

                    if( (!ignore_events_in_list &&  in_event_list) ||
                        ( ignore_events_in_list && !in_event_list) )
                    {
                        // list of events to listen for
                        eventTriggerList.push_back( trigger );
                    }
                }
            }
        }

        return ret;
    }

    std::string ReportEventRecorder::GetHeader() const
    {
        std::stringstream header ;
        header << GetTimeHeader()           << ","
               << "Node_ID"                 << ","
               << "Event_Name"              << ","
               << "Individual_ID"           << ","
               << "Age"                     << ","
               << "Gender"                  << ","
               << "Infected"                << ","
               << "Infectiousness"          ;

        for (const auto& prop : properties_to_report)
        {
            header << "," << prop;
        }

        return header.str();
    }

    bool
    ReportEventRecorder::notifyOnEvent(
        IIndividualHumanEventContext *context,
        const EventTrigger& trigger
    )
    {
        int         id           = context->GetSuid().data;
        ExternalNodeId_t node_id = context->GetNodeEventContext()->GetExternalId();
        IdmDateTime sim_time     = context->GetNodeEventContext()->GetTime();
        const char* event_name   = trigger.c_str();
        float       age          = context->GetAge();
        const char  gender       = (context->GetGender() == Gender::MALE) ? 'M' : 'F' ;
        bool        infected     = context->IsInfected();
        float       infectious   = context->GetInfectiousness() ;


        GetOutputStream() << GetTime( sim_time ) << ","
                          << node_id             << ","
                          << event_name          << ","
                          << id                  << ","
                          << age                 << ","
                          << gender              << ","
                          << infected            << ","
                          << infectious;

        // Report requested properties
        const auto * pProp = context->GetProperties();

        for (const auto& prop_name : properties_to_report)
        {
            IPKey key( prop_name );
            if( !pProp->Contains( key ) )
            {
                throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "properties", prop_name.c_str() );
            }
            GetOutputStream() << "," << pProp->Get( key ).GetValueAsString();
        }

        GetOutputStream() << GetOtherData( context, trigger );
        GetOutputStream() << std::endl;

        return true ;
    }

    std::string ReportEventRecorder::GetTimeHeader() const
    {
        return "Time";
    }

    float ReportEventRecorder::GetTime( const IdmDateTime& rDateTime ) const
    {
        return rDateTime.time;
    }

    std::string ReportEventRecorder::GetOtherData( IIndividualHumanEventContext *context, 
                                                   const EventTrigger& trigger )
    {
        return "" ;
    }
}
