/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include <ctype.h>

#include "stdafx.h"

#include "FileSystem.h"
#include "BaseTextReport.h"
#include "Sugar.h"
#include "IdmMpi.h"

SETUP_LOGGING( "BaseTextReport" )

namespace Kernel {

    BaseTextReport::BaseTextReport( const std::string& rReportName, bool everyTimeStep )
        : BaseReport()
        , write_every_time_step( everyTimeStep )
        , report_name( rReportName )
        , output_stream()
        , reduced_stream()
        , outfile()
        , write_header_newline(true)
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

            FileSystem::OpenFileForWriting( outfile, file_path.c_str() );

            WriteData(GetHeader() + (write_header_newline ? "\n" : ""));
        }
    }

    void BaseTextReport::EndTimestep(float currentTime, float dt)
    {
        if( write_every_time_step )
        {
            GetDataFromOtherCores();
            if( EnvPtr->MPI.Rank == 0 )
            {
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
        if( EnvPtr->MPI.Rank == 0 )
        {
            if( !write_every_time_step )
            {
                WriteData( reduced_stream.str() );
                reduced_stream.str( std::string() ); // clear stream
            }

            outfile.close();
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

        outfile << rStringData ;
    }

    void BaseTextReport::SetReportName(const std::string& new_name)
    {
        // Validates that the requested name contains only alphanumeric characters
        bool good_name = true;
        for(std::string::const_iterator it = new_name.begin(); it != new_name.end(); it++)
        {
            if(!isalnum(*it))
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

