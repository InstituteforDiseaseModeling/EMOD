/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "BaseEventReportIntervalOutput.h"
#include "ReportUtilities.h"
#include "FileSystem.h"
#include "Serializer.h"
#include <math.h>

SETUP_LOGGING( "BaseEventReportIntervalOutput" )

// Idea of this constant is to make sure that the report interval is a multiple of the simulation time step dt. 
// Determining if a float is a multiple of another float is not straight forward because of rounding errors.
// MAX_DEVIATION_REPORTING_INTERVAL_DUE_TO_TIMESTEP is the accepted error, so that after 1000 steps the set interval and the simulation time differ by 1 at max.
#define MAX_DEVIATION_REPORTING_INTERVAL_DUE_TO_TIMESTEP (0.001f)

namespace Kernel
{
    BaseEventReportIntervalOutput::BaseEventReportIntervalOutput()
        : BaseEventReport( "" ) 
        , m_current_time( 0 )
        , m_interval_timer(0)
        , m_reporting_interval(0)
        , m_report_count(0)
        , m_max_number_reports(0)
        , m_one_file_per_report(true)
        , m_has_data(false)
        , m_expired(false)
        , m_PrettyFormat(false)
        , m_pIntervalData(nullptr)
        , m_pMulticoreDataExchange(nullptr)
    {
    }

    BaseEventReportIntervalOutput::BaseEventReportIntervalOutput( const std::string& rReportName, 
                                                                  bool oneFilePerReport,
                                                                  IIntervalData* pIntervalData,
                                                                  IIntervalData* pMulticoreDataExchange )
        : BaseEventReport( rReportName ) 
        , m_current_time(0)
        , m_interval_timer(0)
        , m_reporting_interval(0)
        , m_report_count(0)
        , m_max_number_reports(0)
        , m_one_file_per_report(oneFilePerReport)
        , m_has_data(false)
        , m_expired(false)
        , m_PrettyFormat(false)
        , m_pIntervalData(pIntervalData)
        , m_pMulticoreDataExchange(pMulticoreDataExchange)
    {
        release_assert( m_pIntervalData );
        release_assert( m_pMulticoreDataExchange );
    }

    BaseEventReportIntervalOutput::~BaseEventReportIntervalOutput()
    {
        delete m_pIntervalData;
        m_pIntervalData = nullptr;

        delete m_pMulticoreDataExchange;
        m_pMulticoreDataExchange = nullptr;
    }

    bool BaseEventReportIntervalOutput::Configure( const Configuration * inputJson )
    {
        release_assert( m_pIntervalData );
        release_assert( m_pMulticoreDataExchange );

        initConfigTypeMap( "Reporting_Interval", &m_reporting_interval, Reporting_Interval_DESC_TEXT, 1, 1000000, 1000000);
        initConfigTypeMap( "Max_Number_Reports", &m_max_number_reports, Max_Number_Reports_DESC_TEXT, 0, 1000000, 1);
        initConfigTypeMap( "Pretty_Format",      &m_PrettyFormat,       Pretty_Format_DESC_TEXT, false );

        bool ret = BaseEventReport::Configure( inputJson );
        return ret;
    }

    void BaseEventReportIntervalOutput::EndTimestep( float currentTime, float dt )
    {
        m_current_time = currentTime;

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
            m_interval_timer += dt;

            LOG_DEBUG_F("m_interval_timer=%f, m_reporting_interval=%f\n",m_interval_timer,m_reporting_interval);

            // Check if the defined simulation time step is too big for the defined reporting interval
            if (abs(std::remainder(m_reporting_interval, dt)) > MAX_DEVIATION_REPORTING_INTERVAL_DUE_TO_TIMESTEP)
            {
                LOG_WARN_F("Reporting_Interval ( %f ) is not a multiple of Simulation_Timestep ( %f ), so Time Of Report might not be as expected \n", m_reporting_interval, dt );
            }


            if ( m_interval_timer >= m_reporting_interval )
            {
                LOG_DEBUG_F("Resetting %s reporting interval timer...\n", GetReportName().c_str());
                m_interval_timer -= m_reporting_interval; // allows for dealing with reporting intervals that are not whole days, for example 12 approximately equal reports in a year = 30.42 day interval
                m_report_count++; // have before WriteOutput() so it can use the value

                Reduce();
                if( EnvPtr->MPI.Rank == 0 )
                {
                    AccumulateOutput();
                    if( m_one_file_per_report )
                    {
                        WriteOutput( currentTime );
                        m_has_data = false ;
                    }
                }

                ClearOutputData();

                LOG_DEBUG_F("m_report_count=%d, m_max_number_reports=%d\n",m_report_count,m_max_number_reports);
                if ( m_report_count >= m_max_number_reports )
                {
                    if( (EnvPtr->MPI.Rank == 0) && !m_one_file_per_report )
                    {
                        WriteOutput( currentTime );
                    }
                    m_expired = true;
                    m_has_data = false ;
                    UnregisterAllNodes();
                }
            }
        }
    }

    void BaseEventReportIntervalOutput::Reduce()
    {
        if( !HaveRegisteredAllEvents() )
        {
            return;
        }

        if( EnvPtr->MPI.Rank != 0 )
        {
            IJsonObjectAdapter* pIJsonObj = CreateJsonObjAdapter();
            pIJsonObj->CreateNewWriter();
            pIJsonObj->BeginObject();

            JSerializer js;
            m_pIntervalData->Serialize( *pIJsonObj, js );

            pIJsonObj->EndObject();

            std::string json_data = pIJsonObj->ToString();

            delete pIJsonObj;

            ReportUtilities::SendData( json_data );
        }
        else
        {
            for( int fromRank = 1 ; fromRank < EnvPtr->MPI.NumTasks ; ++fromRank )
            {
                std::vector<char> received;
                ReportUtilities::GetData( fromRank, received );

                IJsonObjectAdapter* pIJsonObj = CreateJsonObjAdapter();
                pIJsonObj->Parse( received.data() );

                m_pMulticoreDataExchange->Clear();
                m_pMulticoreDataExchange->Deserialize( *pIJsonObj );

                m_pIntervalData->Update( *m_pMulticoreDataExchange );

                delete pIJsonObj;
            }
        }
    }

    void BaseEventReportIntervalOutput::Finalize()
    {
        // if the report has data, then we need to write it out at the end.
        if( !m_expired && m_has_data )
        {
            LOG_WARN_F("Report, %s, not written yet, but the simulation is over.  Writing now...\n",GetReportName().c_str());
            AccumulateOutput();
            WriteOutput(-999.0);

            // Probably should clear the output data but it would just waste time
            //ClearOutputData();
        }
        BaseEventReport::Finalize();
    }

    void BaseEventReportIntervalOutput::ClearOutputData()
    {
        if( m_pIntervalData != nullptr )
        {
            m_pIntervalData->Clear();
        }
    }

    std::string BaseEventReportIntervalOutput::CreateOutputFilename( float currentTime )
    {
        std::ostringstream output_file_name;
        output_file_name << GetBaseOutputFilename();
        if( m_one_file_per_report )
        {
            output_file_name << "_" << (m_report_count-1);
        }
        output_file_name << ".json";

        return output_file_name.str();
    }

    void BaseEventReportIntervalOutput::WriteOutput( float currentTime )
    {
        // --------------------------------------
        // --- Serialize data to JSON for output
        // --------------------------------------
        JSerializer js;
        IJsonObjectAdapter* pIJsonObj = CreateJsonObjAdapter();
        pIJsonObj->CreateNewWriter();
        pIJsonObj->BeginObject();

        SerializeOutput( currentTime, *pIJsonObj, js );

        pIJsonObj->EndObject();

        // -------------------------------
        // --- Output JSON to text string
        // -------------------------------
        char* output_text = nullptr;
        if( m_PrettyFormat )
        {
            js.GetPrettyFormattedOutput( pIJsonObj, output_text );
        }
        else
        {
            const char* const_text = nullptr;
            js.GetFormattedOutput( pIJsonObj, const_text );
            output_text = const_cast<char*>(const_text);
        }
        if( output_text == nullptr )
        {
            std::stringstream ss;
            ss << "Error converting JSON to text, base output_file_name=" << GetBaseOutputFilename();
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // --------------------
        // --- Create filename
        // --------------------
        std::string output_file_name = CreateOutputFilename( currentTime );
        output_file_name = FileSystem::Concat( EnvPtr->OutputPath, output_file_name );

        // ------------------------------------
        // --- Open file and write text to file
        // ------------------------------------
        LOG_INFO_F( "Writing file: %s\n", output_file_name.c_str() );
        ofstream ofs;
        ofs.open( output_file_name.c_str() );
        if (!ofs.is_open())
        {
            throw FileIOException( __FILE__, __LINE__, __FUNCTION__, output_file_name.c_str() );
        }

        ofs << output_text << endl;
        if( m_PrettyFormat )
        {
            // the pretty format is an allocated string
            delete output_text ;
        }
        output_text = nullptr ;

        if (ofs.is_open())
        {
            ofs.close();
        }
        pIJsonObj->FinishWriter();
        delete pIJsonObj ;
    }
}

