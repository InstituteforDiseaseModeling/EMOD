/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "IdmApi.h"
#include <string>
#include <fstream>

#include "BoostLibWrapper.h"
#include "CajunIncludes.h"

// common environment information
// This class is intended to be passed around through const reference, so most members will be read only 
// by default. Output streams and the like are mutable because their logical contents won't change.
// I think this falls into the acceptable category: http://www.highprogrammer.com/alan/rants/mutable.html

class SimpleLogger;
class Configuration;
class ValidationLog;
class RANDOMBASE;
class StatusReporter;


class IDMAPI Environment
{
public:
    struct _MPI
    {
        int NumTasks;
        int Rank;
        mutable boost::mpi::communicator *World;
        mutable boost::mpi::environment *Environment;
    } MPI;

    SimpleLogger *Log;
    Configuration *Config;
    void* SimConfig;
    StatusReporter * Status_Reporter;
    
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
    std::string InputPath;
    std::string OutputPath;
    std::string StatePath;
    std::string DllPath;
    RANDOMBASE * RNG;
#pragma warning( pop )

    struct _Report
    {
        ValidationLog mutable *Validation;
    } Report;

    // Sets up the environment for this process. Returns false if something went wrong
    static bool Initialize(
        boost::mpi::environment *mpienv,
        boost::mpi::communicator *world,
        std::string configFileName,
        std::string inputPath,
        std::string outputPath,
// 2.5        std::string statePath,
        std::string dllPath,
        bool get_schema
        );

    // Cleans up open files, handles, memory, etc held by the environment
    static void Finalize();

    static void setLogger(SimpleLogger* log) { if(localEnv == NULL) localEnv = new Environment();  localEnv->Log = log; }
    static const Configuration* getConfiguration() { return localEnv->Config ; }
    static Configuration* CopyFromElement( const json::Element& rElement );
    static Configuration* LoadConfigurationFile( const std::string& rFileName );
    static void setSimulationConfig(void* SimConfig) { localEnv->SimConfig = SimConfig; }
    static const void* getSimulationConfig() { return localEnv->SimConfig; }
    static StatusReporter * getStatusReporter () { return localEnv->Status_Reporter ; }

    // Return path to specified file according to the following order of preference:
    // (1) in current working directory, (2) in specified InputPath
    static std::string FindFileOnPath( const std::string& rFilename );

    static const Environment* getInstance() { return localEnv; }
    static void setInstance(Environment * env) { localEnv = env; }

    virtual ~Environment();

private:
    Environment();

    static Environment * localEnv;
};
