
#include <ctype.h>

#include "stdafx.h"

#include "FileSystem.h"
#include "BaseTextReport.h"
#include "Sugar.h"
#include "IdmMpi.h"

SETUP_LOGGING( "BaseTextReport" )

namespace Kernel {

#define NUM_TIME_STEPS_TO_BUFFER_WRITING (50)

    BaseTextReport::BaseTextReport( const std::string& rReportName, bool everyTimeStep )
        : BaseReport()
        , write_every_time_step( everyTimeStep )
        , write_header_newline(true)
        , num_time_steps(0)
        , report_name( rReportName )
        , output_stream()
        , reduced_stream()
    {
    }

    BaseTextReport::BaseTextReport( const BaseTextReport& rThat )
        : BaseReport( rThat )
        , write_every_time_step( rThat.write_every_time_step )
        , write_header_newline(rThat.write_header_newline)
        , num_time_steps( rThat.num_time_steps )
        , report_name( rThat.report_name )
        , output_stream()
        , reduced_stream()
    {
    }

    BaseTextReport::~BaseTextReport()
    {
    }

    void BaseTextReport::Initialize( unsigned int nrmSize )
    {
        if( EnvPtr->MPI.Rank == 0)
        {
            std::string file_path = GetOutputFilePath();
            if( FileSystem::FileExists( file_path ) )
            {
                FileSystem::RemoveFile( file_path );
            }

            WriteData(GetHeader() + (write_header_newline ? "\n" : ""));
        }
    }

    void BaseTextReport::EndTimestep(float currentTime, float dt)
    {
        if( write_every_time_step )
        {
            num_time_steps++;
            GetDataFromOtherCores();
            if( (EnvPtr->MPI.Rank == 0) && (num_time_steps >= NUM_TIME_STEPS_TO_BUFFER_WRITING) )
            {
                num_time_steps = 0;
                WriteData( reduced_stream.str() );
                reduced_stream.str( std::string() ); // clear stream
            }
        }
    }

    void BaseTextReport::Reduce()
    {
        if( !write_every_time_step )
        {
            GetDataFromOtherCores();
        }
    }

    void BaseTextReport::Finalize()
    {
        if( (EnvPtr->MPI.Rank == 0) && (!write_every_time_step || (num_time_steps > 0)) )
        {
            WriteData( reduced_stream.str() );
            reduced_stream.str( std::string() ); // clear stream
        }
    }

    std::string BaseTextReport::GetOutputFilePath() const
    {
        std::string file_path = FileSystem::Concat( std::string(EnvPtr->OutputPath), GetReportName() );
        return file_path ;
    }
    
    void BaseTextReport::GetDataFromOtherCores()
    {
        std::string to_send = output_stream.str();
        std::string received;

        EnvPtr->MPI.p_idm_mpi->GatherToRoot( to_send, received );
        if (EnvPtr->MPI.Rank == 0)
        {
            reduced_stream << received;
        }

        output_stream.str(std::string());   // Clear the output stream.
    }

    void BaseTextReport::WriteData( const std::string& rStringData )
    {
        if( rStringData.empty() )
        {
            return ;
        }

        // ------------------------------------------------------------------------------------------------
        // --- Previously, we open the file during initialization and closed during finalize.
        // --- Users were seeing times when the ReportEventRecorder.csv would have no data even though
        // --- the header was written during initialization.  Hence, we are changing to open and closing
        // --- for each write to the file.  This should only happen once per timestep.
        // --- Performance comparision has this being 3-5% slower.
        // --- I considered flush() but this seems much safer.
        // ------------------------------------------------------------------------------------------------
        ofstream outfile;
        FileSystem::OpenFileForWriting( outfile, GetOutputFilePath().c_str(), false, true );
        outfile << rStringData ;
        outfile.close();
    }

    void BaseTextReport::SetReportName(const std::string& new_name)
    {
        // Validates that the requested name contains only alphanumeric characters
        bool good_name = true;
        for(std::string::const_iterator it = new_name.begin(); it != new_name.end(); it++)
        {
            if( !isalnum(*it) && (*it != '_') && (*it != '-') && (*it != '.') )
            {
                good_name = false;
                break;
            }
        }

        // Only updates the reporter with a new name if it passed validation
        if(good_name)
        {
            report_name = new_name;
        }
        else
        {
            LOG_WARN("Report name is not alphanumeric; default name used.\n");
        }

        return;
    }

    void BaseTextReport::AddHeaderLine(bool add_endl)
    {
        write_header_newline = add_endl;

        return;
    }
}

