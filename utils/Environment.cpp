/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "StatusReporter.h"
#include <errno.h>
#include <iostream>
#include "Environment.h"
#include "FileSystem.h"
#include "Exceptions.h"
#include "Configuration.h"
#include "Configure.h"
#include "Log.h"
#include "ValidationLog.h"

#include <iso646.h>

using namespace std;

static const char * _module = "Environment";

#pragma warning(disable: 4996) // for suppressing strcpy caused security warnings

Environment* Environment::localEnv = NULL;

Environment::Environment()
: Config(NULL)
, SimConfig(NULL)
, InputPath()
, OutputPath()
, StatePath()
, DllPath()
{
    Report.Validation = NULL;
}

bool Environment::Initialize(
    boost::mpi::environment *mpienv, boost::mpi::communicator *world, 
    string configFileName, 
    string inputPath, string outputPath, /* 2.5 string statePath, */ string dllPath,
    bool get_schema)
{
    localEnv->MPI.NumTasks = world->size();
    localEnv->MPI.Rank = world->rank();
    localEnv->MPI.World = world;
    localEnv->MPI.Environment = mpienv;


    inputPath = FileSystem::RemoveTrailingChars( inputPath );
    if( !FileSystem::DirectoryExists(inputPath) )
    {
        LOG_WARN_F("Input path %s doesn't exist\n", inputPath.c_str());
    }

    outputPath = FileSystem::RemoveTrailingChars( outputPath );

    if( !get_schema )
    {
        if( FileSystem::FileExists( outputPath ) )
        {
            std::string msg = "Output Path ("+outputPath+") exists and is not a directory." ;
            throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.c_str() );
        }

        if( localEnv->MPI.Rank != 0 )
        {
            MPI_Barrier( MPI_COMM_WORLD );
        }
        else
        {
            if( !FileSystem::DirectoryExists(outputPath) )
            {
                FileSystem::MakeDirectory( outputPath ) ;
            }
            MPI_Barrier( MPI_COMM_WORLD );
        }

        if( !FileSystem::DirectoryExists(outputPath) )
        {
            LOG_ERR_F( "Failed to create new output directory %s with error %s\n", localEnv->OutputPath.c_str(), strerror(errno) );
            throw Kernel::FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, localEnv->OutputPath.c_str() );
        }
    }

    localEnv->OutputPath  = outputPath;
    localEnv->InputPath   = inputPath;
// 2.5    localEnv->StatePath   = statePath;
    localEnv->DllPath     = dllPath;

    if( get_schema )
    {
        return true;
    }

    Configuration *config = Configuration::Load(configFileName);

    if (!config) 
    {
        delete localEnv;
        localEnv = NULL;
        throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, configFileName.c_str() );
    }

    localEnv->Log->Init( config );

    localEnv->Config = Configuration::CopyFromElement((*config)["parameters"]);

    if( localEnv->Config->CheckElementByName("Default_Config_Path") || config->CheckElementByName("Default_Config_Path") )
        Kernel::JsonConfigurable::_possibleNonflatConfig = true;

    localEnv->Report.Validation = ValidationLog::CreateNull(); // eliminate some overhead by creating a dummy object that does nothing

    localEnv->Status_Reporter = StatusReporter::getInstance();

    return true;
}

Environment::~Environment()
{
    if (Config)
        delete Config;

    if (Report.Validation)
        delete Report.Validation;
}

void Environment::Finalize()
{
    if (localEnv)
        delete localEnv;
}

std::string Environment::FindFileOnPath( const std::string& rFilename )
{
    if( FileSystem::FileExists( FileSystem::Concat(FileSystem::GetCurrentWorkingDirectory(), rFilename) ) )
    {
        return rFilename;
    }
    std::string filepath = FileSystem::Concat( localEnv->InputPath, rFilename );
    if( FileSystem::FileExists( filepath ) )
    {
        return filepath;
    }
    else
    {
        throw Kernel::FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, filepath.c_str() );
    }
}

Configuration* Environment::CopyFromElement( const json::Element& rElement )
{
    return Configuration::CopyFromElement( rElement );
}

Configuration* Environment::LoadConfigurationFile( const std::string& rFileName )
{
    return Configuration::Load( rFileName );
}
