/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IdmApi.h"
#include <string>
#include <fstream>
#include <map>

#include "BoostLibWrapper.h"
#include "CajunIncludes.h"

// common environment information
// This class is intended to be passed around through const reference, so most members will be read only 
// by default. Output streams and the like are mutable because their logical contents won't change.
// I think this falls into the acceptable category: http://www.highprogrammer.com/alan/rants/mutable.html

class SimpleLogger;
class Configuration;
class ValidationLog;
class StatusReporter;

namespace IdmMpi
{
    class MessageInterface;
}


class IDMAPI Environment
{
public:
    struct _MPI
    {
        int NumTasks;
        int Rank;
        IdmMpi::MessageInterface* p_idm_mpi;
    } MPI;

    SimpleLogger *Log;
    Configuration *Config;
    void* SimConfig;
    void* pIPFactory;
    void* pNPFactory;
    void* pRngFactory;
    std::vector<void*> event_trigger_factories;
    StatusReporter * Status_Reporter;
    
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
    std::list< std::string > InputPaths;
    std::string OutputPath;
    std::string StatePath;
    std::string DllPath;
#pragma warning( pop )

    struct _Report
    {
        ValidationLog mutable *Validation;
    } Report;

    // Sets up the environment for this process. Returns false if something went wrong
    static bool Initialize(
        IdmMpi::MessageInterface* pMpi,
        std::string configFileName,
        std::string inputPath,
        std::string outputPath,
// 2.5        std::string statePath,
        std::string dllPath,
        bool get_schema
        );

    // Cleans up open files, handles, memory, etc held by the environment
    static void Finalize();

    static const Configuration* getConfiguration();
    static Configuration* CopyFromElement( const json::Element& rElement, const std::string& rDataLocation = "Unknown" );
    static Configuration* LoadConfigurationFile( const std::string& rFileName );

    static Environment* getInstance();
    static void setInstance( Environment * env );
    static void setLogger( SimpleLogger* log );
    static void setSimulationConfig( void* SimConfig );
    static const void* getSimulationConfig();
    static StatusReporter * getStatusReporter();
    static void setIPFactory( void* pipf );
    static void* getIPFactory();
    static void setNPFactory( void* pnpf );
    static void* getNPFactory();

    static const void* getEventTriggerFactory( int event_type );
    static void setEventTriggerFactory( int event_type, void* pFactory );

    static const void* getRandomNumberGeneratorFactory();
    static void setRandomNumberGeneratorFactory( void* pFactory );

    // Return path to specified file according to the following order of preference:
    // (1) in current working directory, (2) in specified InputPath
    static std::string FindFileOnPath( const std::string& rFilename );


    virtual ~Environment();

private:
    Environment();

    //static Environment * localEnv;
    static Environment * & getInstanceAux();
};
