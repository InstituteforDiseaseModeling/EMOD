
/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "BaseEventReportIntervalOutput.h"

static const char * _module = "BaseEventReportIntervalOutput"; 



namespace Kernel
{
    BaseEventReportIntervalOutput::BaseEventReportIntervalOutput( const std::string& rReportName )
        : BaseEventReport( rReportName ) 
        , m_interval_timer(0)
        , m_reporting_interval(0)
        , m_report_count(0)
        , m_max_number_reports(0)
    {
    }

    BaseEventReportIntervalOutput::~BaseEventReportIntervalOutput()
    {
    }

    bool BaseEventReportIntervalOutput::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Reporting_Interval", &m_reporting_interval, Reporting_Interval_DESC_TEXT, 0, 1000000, 1000000);
        initConfigTypeMap("Max_Number_Reports", &m_max_number_reports, Max_Number_Reports_DESC_TEXT, 0, 1000000, 1);

        bool ret = BaseEventReport::Configure( inputJson );
        return ret;
    }

    void BaseEventReportIntervalOutput::EndTimestep( float currentTime, float dt )
    {
        if( HaveUnregisteredAllEvents() )
        {
            // --------------------------------------------------------------------------------
            // --- If we have either not registered or unregistered listening for events, then
            // --- we don't want to consider outputing data.
            // --------------------------------------------------------------------------------
            return;
        }
        else if( HaveRegisteredAllEvents() )
        {
            m_interval_timer++;

            LOG_DEBUG_F("m_interval_timer=%d, m_reporting_interval=%d\n",m_interval_timer,m_reporting_interval);
            if ( m_interval_timer >= m_reporting_interval )
            {
                m_report_count++;

                LOG_DEBUG("Timer has reached reporting interval.  Writing report...\n");
                WriteOutput( currentTime );

                LOG_DEBUG_F("Resetting %s reporting interval timer...\n", GetReportName().c_str());
                m_interval_timer = 0 ;
                ClearOutputData();

                if ( m_report_count >= m_max_number_reports )
                {
                    UnregisterAllNodes();
                }
            }
        }
    }

    void BaseEventReportIntervalOutput::Finalize()
    {
        // If all defaults are used, we won't have written the report during runtime, so write it out at end.
        if( m_report_count == 0 )
        {
            LOG_WARN_F("Report, %s, not written yet, but the simulation is over.  Writing now...\n",GetReportName().c_str());
            WriteOutput(-999.0);
            ClearOutputData();
        }
        BaseEventReport::Finalize();
    }
}

