/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReportSurveillanceEventRecorder.h"
#include "Log.h"
#include "Exceptions.h"
#include "EventCoordinator.h"
#include "SimulationEventContext.h"
#include "EventTriggerCoordinator.h"
#include "ISurveillanceReporting.h"

SETUP_LOGGING( "ReportSurveillanceEventRecorder" )

namespace Kernel
{
    const std::string SURV_ENABLE_PARAMETER_NAME   = "Report_Surveillance_Event_Recorder";
    const std::string SURV_EVENTS_LIST_NAME        = "Report_Surveillance_Event_Recorder_Events";
    const std::string SURV_EVENTS_LIST_DESC        =  Report_Surveillance_Event_Recorder_Events_DESC_TEXT;
    const std::string SURV_IGNORE_EVENTS_LIST_NAME = "Report_Surveillance_Event_Recorder_Ignore_Events_In_List";
    const std::string SURV_IGNORE_EVENTS_LIST_DESC =  Report_Surveillance_Event_Recorder_Ignore_Events_In_List_DESC_TEXT;

    const std::string STATS_BY_IP_PARAMETER_NAME = "Report_Surveillance_Event_Recorder_Stats_By_IPs";

    GET_SCHEMA_STATIC_WRAPPER_IMPL( ReportSurveillanceEventRecorder, ReportSurveillanceEventRecorder )

    std::string ReportSurveillanceEventRecorder::GetEnableParameterName()
    {
        return SURV_ENABLE_PARAMETER_NAME;
    }

    IReport* ReportSurveillanceEventRecorder::CreateReport()
    {
        return new ReportSurveillanceEventRecorder();
    }

    ReportSurveillanceEventRecorder::ReportSurveillanceEventRecorder()
        : ReportEventRecorderCoordinator( "ReportSurveillanceEventRecorder.csv" )
        , m_StatsByIpKeyNames()
        , m_ReportStatsByIP()
    {
        m_EnableParameterName  = SURV_ENABLE_PARAMETER_NAME;
        m_EventsListName       = SURV_EVENTS_LIST_NAME;
        m_EventsListDesc       = SURV_EVENTS_LIST_DESC;
        m_IgnoreEventsListName = SURV_IGNORE_EVENTS_LIST_NAME;
        m_IgnoreEventsListDesc = SURV_IGNORE_EVENTS_LIST_DESC;
    }

    ReportSurveillanceEventRecorder::~ReportSurveillanceEventRecorder()
    {
    }

    bool ReportSurveillanceEventRecorder::Configure( const Configuration* inputJson )
    {
        m_StatsByIpKeyNames.value_source = IPKey::GetConstrainedStringConstraintKey();
        initConfigTypeMap( STATS_BY_IP_PARAMETER_NAME.c_str(), &m_StatsByIpKeyNames, Report_Surveillance_Event_Recorder_Stats_By_IPs_DESC_TEXT, ENABLE_PARAMETER_NAME.c_str() );

        return ReportEventRecorderCoordinator::Configure( inputJson );
    }

    void ReportSurveillanceEventRecorder::Initialize( unsigned int nrmSize )
    {
        m_ReportStatsByIP.SetIPKeyNames( STATS_BY_IP_PARAMETER_NAME, m_StatsByIpKeyNames );

        ReportEventRecorderCoordinator::Initialize( nrmSize );
    }

    std::string ReportSurveillanceEventRecorder::GetHeader() const
    {
        std::stringstream header;
        header << ReportEventRecorderCoordinator::GetHeader()
               << "," << "NumCounted"
               << "," << "ThresholdOfAction"
               << "," << m_ReportStatsByIP.GetHeader();

        return header.str();
    }

    bool ReportSurveillanceEventRecorder::notifyOnEvent( IEventCoordinatorEventContext *pEntity, const EventTriggerCoordinator& trigger )
    {
        ISurveillanceReporting* p_isr = nullptr;
        if( pEntity->QueryInterface( GET_IID( ISurveillanceReporting ), (void**)&p_isr ) == s_OK )
        {
            return ReportEventRecorderCoordinator::notifyOnEvent( pEntity, trigger );
        }
        else
        {
            return false;
        }
    }

    std::string ReportSurveillanceEventRecorder::GetOtherData( IEventCoordinatorEventContext *pEntity,
                                                              const EventTriggerCoordinator& trigger )
    {
        ISurveillanceReporting* p_isr = nullptr;
        if( pEntity->QueryInterface( GET_IID( ISurveillanceReporting ), (void**)&p_isr ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "pEntity", "ISurveillanceReporting", "IEventCoordinatorEventContext" );
        }

        m_ReportStatsByIP.ResetData();

        p_isr->CollectStats( m_ReportStatsByIP );

        std::stringstream ss;

        ss << ReportEventRecorderCoordinator::GetOtherData( pEntity, trigger )
           << "," << p_isr->GetNumCounted()
           << "," << p_isr->GetCurrentActionThreshold()
           << "," << m_ReportStatsByIP.GetReportData();

        return ss.str();
    }
}
