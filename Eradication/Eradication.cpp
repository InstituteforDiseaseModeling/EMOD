/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#pragma warning(disable:4996)

#include <iostream>
#include <fstream>
#include <sstream> // ostringstream

#include <math.h>
#include <stdio.h>

#include "Environment.h"
#include "FileSystem.h"
#include "BoostLibWrapper.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "ProgramOptions.h"
#include "Controller.h"
#include "Debug.h"
#include "ControllerFactory.h"
#include "StatusReporter.h"
#include "PythonSupport.h"

#include "Exceptions.h"

// Version info extraction for both program and dlls
#include "ProgVersion.h"
#include "SimulationConfig.h"
#include "InterventionFactory.h"
#include "EventCoordinator.h"
#include "IWaningEffect.h"
#include "CampaignEvent.h"
#include "StandardEventCoordinator.h"
#include "DllLoader.h"
#include "IdmMpi.h"
#include "IdmString.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! By creating an instance of this object, we ensure that the linker does not optimize it away.
// !!! This allows the componentTests to test these classes even though they are mostly used by DLLs.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#include "BaseEventReportIntervalOutput.h"
Kernel::BaseEventReportIntervalOutput junk("");  // make linker keep
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


using namespace std;

int MPIInitWrapper(int argc, char* argv[]);


#ifndef WIN32
void setStackSize();
#define sprintf_s sprintf
void setStackSize();
#endif

static const char* _module = "Eradication";

void Usage(char* cmd)
{
    LOG_INFO_F("For full usage, run: %s --help\n", cmd);
    exit(1);
}

    const std::vector<std::string> 
    getSimTypeList()
    {
    const char * simTypeListC[] = { "GENERIC_SIM"
#ifndef DISABLE_VECTOR
        , "VECTOR_SIM"
#endif
#ifndef DISABLE_MALARIA
            , "MALARIA_SIM"
#endif
#ifndef DISABLE_AIRBORNE
            , "AIRBORNE_SIM"
#endif
#ifdef ENABLE_POLIO
            , "POLIO_SIM"
#endif
#ifdef ENABLE_TB
            , "TB_SIM"
#endif
#ifdef ENABLE_TBHIV
            , "TBHIV_SIM"
#endif
#ifndef DISABLE_STI
            , "STI_SIM"
#endif
#ifndef DISABLE_HIV
            , "HIV_SIM"
#endif
#ifdef ENABLE_PYTHON_FEVER
            , "PY_SIM"
#endif
    };

#define KNOWN_SIM_COUNT (sizeof(simTypeListC)/sizeof(simTypeListC[0]))
    std::vector<std::string> simTypeList( simTypeListC, simTypeListC + KNOWN_SIM_COUNT );
    return simTypeList;
    }

int main(int argc, char* argv[])
{
    // First thing to do when app launches
    Environment::setLogger(new SimpleLogger());
    if (argc < 2)
    {
        Usage(argv[0]);
    }

#ifndef WIN32
    setStackSize();
#endif

    ProgDllVersion* pv = new ProgDllVersion(); // why new this instead of just put it on the stack?
    auto sims = getSimTypeList();
    std::stringstream output;
    output << "Intellectual Ventures(R)/EMOD Disease Transmission Kernel " << pv->getVersion() << std::endl
           << "Built on " << pv->getBuildDate() <<
           " by " << pv->getBuilderName() <<
           " from " << pv->getSccsBranch() <<
           " checked in on " << pv->getSccsDate() << std::endl;
    
    std::string sim_types_str = "Supports sim_types: ";
    for( auto sim_type: sims  )
    {
        sim_types_str += IdmString( sim_type ).split('_')[0];
        sim_types_str += ", ";
    }
    sim_types_str.pop_back();
    sim_types_str.pop_back();
    output << sim_types_str << "." << std::endl;
    LOG_INFO_F( output.str().c_str() );
    delete pv;
 
/* // for debugging all kernel allocations, use inner block around controller lifetime to ignore some environment and mpi stuff
#ifdef WIN32
#ifdef _DEBUG
    // In release builds, _CrtMemCheckpoint and _CrtMemDumpAllObjectsSince below are no-ops
    _CrtMemState initial_state;
#endif
    _CrtMemCheckpoint(&initial_state);

    //_crtBreakAlloc = 10242; // set this to alloc number to break on
#endif
*/
    int ret = MPIInitWrapper(argc, argv);
/*
#ifdef WIN32
    _CrtMemDumpAllObjectsSince(&initial_state);
#endif
*/
    return ret;
}

#ifndef WIN32

#define sprintf_s sprintf   

// hacks to give us some more stack space size since there isn't a compiler/linker option
#include <sys/resource.h>
void setStackSize()
{
    const rlim_t kStackSize = 32 * 1024 * 1024;   // min stack size = 32 MB
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            }
        }
    }
}

#endif


bool ControllerInitWrapper(int argc, char *argv[], IdmMpi::MessageInterface* pMpi ); // returns false if something broke

int MPIInitWrapper( int argc, char* argv[])
{
    try 
    {
        bool fSuccessful = false;
        IdmMpi::MessageInterface* p_mpi = IdmMpi::MessageInterface::Create( argc, argv );

        try 
        {
            fSuccessful = ControllerInitWrapper( argc, argv, p_mpi );
        }
        catch( std::exception& e) 
        {
            LOG_ERR_F("Error in ControllerInitWrapper: %s\n", e.what());
        }

        // If ControllerInitWrapper() returned false (error) or an exception was
        // caught, tear everything down, decisively.
        if (!fSuccessful)
        {
            if( EnvPtr != nullptr )
            {
                EnvPtr->Log->Flush();
            }
            p_mpi->Abort(-1);
        }

        p_mpi->Finalize();
        delete p_mpi;
        p_mpi = nullptr;

        // Shouldn't get here unless ControllerInitWrapper() returned true (success).
        // MPI_Abort() implementations generally exit the process. If not, return a
        // useful value.
        return fSuccessful ? 0 : -1;
    }
    catch( std::exception &e )
    {
        LOG_ERR_F("Error in MPIInitWrapper: %s\n", e.what());
        return -1;
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////
// access to input schema embedding

void IDMAPI writeInputSchemas(
    const char* dll_path,
    const char* output_path
)
{
    std::ofstream schema_ostream_file;
    json::Object fakeJsonRoot;
    json::QuickBuilder total_schema( fakeJsonRoot );

    // Get DTK version into schema
    json::Object vsRoot;
    json::QuickBuilder versionSchema( vsRoot );
    ProgDllVersion pv;
    versionSchema["DTK_Version"] = json::String( pv.getVersion() );
    versionSchema["DTK_Branch"] = json::String( pv.getSccsBranch() );
    versionSchema["DTK_Build_Date"] = json::String( pv.getBuildDate() );
    total_schema["Version"] = versionSchema.As<json::Object>();

    std::string szOutputPath = std::string( output_path );
    if( szOutputPath != "stdout" )
    {
        // Attempt to open output path
        schema_ostream_file.open( output_path, std::ios::out );
        if( schema_ostream_file.fail() )
        {
            LOG_ERR_F( "Failed to open: %s\n", output_path );
            return;
        }
    }
    std::ostream &schema_ostream = ( ( szOutputPath == "stdout" ) ? std::cout : schema_ostream_file );

    DllLoader dllLoader;
    std::map< std::string, createSim > createSimFuncPtrMap;

    if( dllLoader.LoadDiseaseDlls(createSimFuncPtrMap) )
    {
        //LOG_DEBUG( "Calling GetDiseaseDllSchemas\n" );
        json::Object diseaseSchemas = dllLoader.GetDiseaseDllSchemas();
        total_schema[ "config:emodules" ] = diseaseSchemas;
    }

    // Intervention dlls don't need any schema printing, just loading them
    // through DllLoader causes them to all get registered with exe's Intervention
    // Factory, which makes regular GetSchema pathway work. This still doesn't solve
    // the problem of reporting in the schema output what dll they came from, if we
    // even want to.
    dllLoader.LoadInterventionDlls();

    Kernel::JsonConfigurable::_dryrun = true;
    std::ostringstream oss;

    
    json::Object configSchemaAll;
    for (auto& sim_type : getSimTypeList())
    {
        json::Object fakeSimTypeConfigJson;
        fakeSimTypeConfigJson["Simulation_Type"] = json::String(sim_type);
        Configuration * fakeConfig = Configuration::CopyFromElement( fakeSimTypeConfigJson );
        Kernel::SimulationConfig * pConfig = Kernel::SimulationConfigFactory::CreateInstance(fakeConfig);
        release_assert( pConfig );

        json::QuickBuilder config_schema = pConfig->GetSchema();
        configSchemaAll[sim_type] = config_schema;

        delete fakeConfig;
        fakeConfig = nullptr;
    }

    for (auto& entry : Kernel::JsonConfigurable::get_registration_map())
    {
        const std::string& classname = entry.first;
        json::QuickBuilder config_schema = ((*(entry.second))());
        configSchemaAll[classname] = config_schema;
    }

    total_schema[ "config" ] = configSchemaAll;

    if( !Kernel::InterventionFactory::getInstance() )
    {
        throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, "Kernel::InterventionFactory::getInstance(" );
    }

    json::QuickBuilder ces_schema = Kernel::CampaignEventFactory::getInstance()->GetSchema();
    json::QuickBuilder ecs_schema = Kernel::EventCoordinatorFactory::getInstance()->GetSchema();
    json::QuickBuilder ivs_schema = Kernel::InterventionFactory::getInstance()->GetSchema();
    json::QuickBuilder ns_schema  = Kernel::NodeSetFactory::getInstance()->GetSchema();
    json::QuickBuilder we_schema  = Kernel::WaningEffectFactory::getInstance()->GetSchema();

    json::Object objRoot;
    json::QuickBuilder camp_schema( objRoot );

    json::Object useDefaultsRoot;
    json::QuickBuilder udSchema( useDefaultsRoot );
    udSchema["type"] = json::String( "bool" );
    udSchema["default"] = json::Number( 0 );
    udSchema["description"] = json::String( CAMP_Use_Defaults_DESC_TEXT );
    camp_schema["Use_Defaults"] = udSchema.As<json::Object>();

    camp_schema["Events"][0] = json::String( "idmType:CampaignEvent" );

    camp_schema["idmTypes" ][ "idmType:CampaignEvent" ] = ces_schema.As<json::Object>();
    camp_schema["idmTypes" ][ "idmType:EventCoordinator" ] = ecs_schema.As<json::Object>();
    camp_schema["idmTypes" ][ "idmType:Intervention"] = ivs_schema.As<json::Object>();
    camp_schema["idmTypes" ][ "idmType:NodeSet"] = ns_schema[ "schema" ].As<json::Object>();
    camp_schema["idmTypes" ][ "idmType:WaningEffect"] = we_schema[ "schema" ].As<json::Object>();

    total_schema[ "interventions" ] = camp_schema.As< json::Object> ();
    json::Writer::Write( total_schema, schema_ostream );
    schema_ostream_file.close();

    // PythonSupportPtr can be null during componentTests
    if( (szOutputPath != "stdout") && (PythonSupportPtr != nullptr) )
    {
        PythonSupportPtr->RunPostProcessSchemaScript( output_path );
    }
}

bool ControllerInitWrapper( int argc, char *argv[], IdmMpi::MessageInterface* pMpi )
{
    using namespace std;

    /////////////////////////////////////////////////////////////
    // parse input arguments

    ProgramOptions po("Recognized options");
    try
    {
        po.AddOption(          "help",         "Show this help message." );
        po.AddOption(          "version",      "v",          "Get version info." );
        po.AddOption(          "get-schema",   "Request the kernel to write all its input definition schema json to the current working directory and exit." );
        po.AddOptionWithValue( "schema-path",  "stdout",     "Path to write schema(s) to instead of writing to stdout." );
        po.AddOptionWithValue( "config",       "C",          "default-config.json", "Name of config.json file to use" );
        po.AddOptionWithValue( "input-path",   "I",          ".",                   "Relative or absolute path to location of model input files" );       
        po.AddOptionWithValue( "output-path",  "O",          "output",              "Relative or absolute path for output files" );
        po.AddOptionWithValue( "dll-path",     "D",          "",                    "Relative (to the executable) or absolute path for EMODule (dll/shared object) root directory" );
// 2.5        po.AddOptionWithValue( "state-path",   "S",          ".",                   "Relative or absolute path for state files" );  // should we remove this...?
        po.AddOptionWithValue( "monitor_host", "none",       "IP of commissioning/monitoring host" );
        po.AddOptionWithValue( "monitor_port", 0,            "port of commissioning/monitoring host" );
        po.AddOptionWithValue( "sim_id",       "none",       "Unique id of this simulation, formerly sim_guid. Needed for self-identification to UDP host" );
        po.AddOption(          "progress",     "Send updates on the progress of the simulation to the HPC job scheduler." );
#ifdef ENABLE_PYTHON
        po.AddOptionWithValue( "python-script-path", "P", "", "Path to python scripts." );
#endif

        std::string errmsg = po.ParseCommandLine( argc, argv );
        if( !errmsg.empty() )
        {
            LOG_ERR_F("Error parsing command line: %s\n", errmsg.c_str() );
            return false;
        }

        if( po.CommandLineHas( "help" ) )
        {
            std::ostringstream oss;
            po.Print( oss );
            LOG_INFO(oss.str().c_str());
            return true;
        }

        if( po.CommandLineHas( "version" ) )
        {
            ProgDllVersion pv;
            std::ostringstream oss;
            oss << argv[0] << " version: " << pv.getVersion()  << std::endl;
            std::list<string> dllNames;
            std::list<string> dllVersions;
#ifdef WIN32

            if( po.CommandLineHas( "dll-path" ) )
            {
                DllLoader dllLoader;
                if( dllLoader.GetEModulesVersion( po.GetCommandLineValueString( "dll-path" ).c_str(), dllNames, dllVersions ) )
                {
                    std::list<string>::iterator iter1;
                    std::list<string>::iterator iter2;
                    for(iter1 = dllNames.begin(),iter2 = dllVersions.begin(); 
                        iter1 != dllNames.end();
                        ++iter1, ++iter2)
                    {
                        oss << *iter1 << " version: " << *iter2 << std::endl;
                    }
                }
            }
            else
            {
                LOG_DEBUG("The EModule root path is not given, so nothing to get version from.\n");
            }
#endif
            LOG_INFO(oss.str().c_str());
            return true;
        }

        //////////////////////////////////////////
        if( po.CommandLineHas( "config" ) )
            LOG_INFO_F( "Using config file: %s\n", po.GetCommandLineValueString( "config" ).c_str() );
        else 
            LOG_WARN("No config file specified.\n");

        if( po.CommandLineHas( "input-path" ) )
            LOG_INFO_F( "Using input path: %s\n", po.GetCommandLineValueString( "input-path" ).c_str() );
        else 
            LOG_WARN("Input path not specified, assuming current directory.\n"); // in reality these never happen since default values are supplied by the program options

        if( po.CommandLineHas( "output-path" ) )
            LOG_INFO_F( "Using output path: %s\n", po.GetCommandLineValueString( "output-path" ).c_str() );
        else 
            LOG_WARN("Output path not specified, using current directory.\n");

        if( po.CommandLineHas( "dll-path" ) )
            LOG_INFO_F( "using dll path: %s\n", po.GetCommandLineValueString( "dll-path" ).c_str() );
        else
            LOG_WARN("Dll path not specified.\n");
    }
    catch( std::exception& e) 
    {
        LOG_ERR_F("Error parsing command line: %s\n", e.what());
        return false;
    }
    catch(...) 
    {
        LOG_ERR("Exception of unknown type!\n");
        return false;
    }
    EnvPtr->Log->Flush();

    bool is_getting_schema = po.CommandLineHas( "get-schema" );

    // --------------------------------------------------------------------------------------------------
    // --- DMB 9-9-2014 ERAD-1621 Users complained that a file not found exception on default-config.json
    // --- really didn't tell them that they forgot to specify the configuration file on the command line.
    // --- One should be able to get the schema without specifying a config file.
    // --------------------------------------------------------------------------------------------------
    if( (po.GetCommandLineValueString( "config" ) == po.GetCommandLineValueDefaultString( "config" )) &&
       !is_getting_schema )
    {
        if( !FileSystem::FileExists( po.GetCommandLineValueDefaultString( "config" ) ) )
        {
            std::string msg ;
            msg += "The configuration file '" + po.GetCommandLineValueDefaultString( "config" ) + "' could not be found.  " ;
            msg += "Did you forget to define the configuration file on the command line with --config or -C?" ;
            LOG_ERR_F( msg.c_str() );
            return false ;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    // Run model

    // handle run modes

    bool status = false;
    ostringstream exceptionErrorReport;
    try
    {
        if( po.CommandLineHas( "progress" ) )
        {
            StatusReporter::updateScheduler = true;
        }

        auto configFileName = po.GetCommandLineValueString( "config" );

        auto schema_path = po.GetCommandLineValueString("schema-path");

        std::string python_script_path = "";
#ifdef ENABLE_PYTHON
        python_script_path = po.GetCommandLineValueString( "python-script-path" );
        if( python_script_path == "" )
        {
            std::cout << "--python-script-path (-P) not on command line - not using embedded python" << std::endl;
        }
        else
        {
            if( is_getting_schema && (schema_path == "stdout") )
            {
                throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, 
                    "--schema-path=stdout and --python-script-path is defined:  the post processing script can only work on a file.  Please define --schema-path as a filename." );
            }
            std::cout << "python_script_path = " << python_script_path << std::endl;
        }
#endif
        
        Kernel::PythonSupport* p_python_support = new Kernel::PythonSupport();
        p_python_support->CheckPythonSupport( is_getting_schema, python_script_path );

        // Where to put config preprocessing? After command-line processing (e.g., what if we are invoked with --get-schema?) but before loading config.json
        // Prefer to put it in this function rather than inside Environment::Intialize, but note that in --get-schema mode we'll unnecessarily call preproc.
        // NOTE: This enables functionality to allow config.json to be specified in (semi-)arbitrary formats as long as python preproc's into standard config.json
        configFileName = p_python_support->RunPreProcessScript( configFileName );

        // set up the environment
        EnvPtr->Log->Flush();
        LOG_INFO("Initializing environment...\n");
        bool env_ok = Environment::Initialize(
            pMpi,
            p_python_support,
            configFileName,
            po.GetCommandLineValueString( "input-path"  ),
            po.GetCommandLineValueString( "output-path" ),
// 2.5            po.GetCommandLineValueString( "state-path"  ),
            po.GetCommandLineValueString( "dll-path"    ),
            is_getting_schema
            );

        if (!env_ok)
        {
            LOG_ERR("Failed to initialize environment, exiting\n");
            return false;
        }

        if( is_getting_schema )
        {
            writeInputSchemas( po.GetCommandLineValueString( "dll-path" ).c_str(), po.GetCommandLineValueString( "schema-path" ).c_str() );
            return true;
        }

        // check if we can support python scripts based on simulation type
        std::string sim_type_str = GET_CONFIG_STRING( EnvPtr->Config, "Simulation_Type" );
        PythonSupportPtr->CheckSimScripts( sim_type_str );


        // UDP-enabled StatusReporter needs host and sim unique id.
        if( po.GetCommandLineValueString( "monitor_host" ) != "none" )
        {
            LOG_INFO_F("Status monitor host: %s\n", po.GetCommandLineValueString( "monitor_host" ).c_str() );
            EnvPtr->getStatusReporter()->SetHost( po.GetCommandLineValueString( "monitor_host" ) );
        }

        if( po.GetCommandLineValueInt( "monitor_port" ) != 0 )
        {
            LOG_INFO_F("Status monitor port: %d \n",  po.GetCommandLineValueInt( "monitor_port" )  );
            EnvPtr->getStatusReporter()->SetMonitorPort(  po.GetCommandLineValueInt( "monitor_port" ) );
        }

        if( po.GetCommandLineValueString( "sim_id" ) != "none" )
        {
            LOG_INFO_F("Status monitor sim id: %s\n", po.GetCommandLineValueString( "sim_id" ).c_str() );
            EnvPtr->getStatusReporter()->SetHPCId( po.GetCommandLineValueString( "sim_id" ) );
        }

#ifdef _DEBUG
    //#define DEBUG_MEMORY_LEAKS // uncomment this to run an extra warm-up pass and enable dumping objects 
#endif 

#ifdef WIN32

    #ifdef DEBUG_MEMORY_LEAKS
        _CrtMemState initial_state;

        for (int run = 0; run < 2; run++)
        {
            if (run == 1)
            {
                _RPT0(_CRT_WARN,"Beginning check pass...\n");

                _CrtMemCheckpoint(&initial_state);
                int *intential_leak_marker = _new_ int[250]; // help us locate beginning of allocations during the check pass because DumpAllObjectsSince is not reliable and sometimes dumps everything

            //    _crtBreakAlloc = 106768; // break on this alloc number; get this from the object dump
            }
    #endif

#endif

        LOG_INFO( "Loaded Configuration...\n" ); 
        //LOG_INFO_F( "Name: %s\n", GET_CONFIGURABLE(SimulationConfig)->ConfigName.c_str() );  // can't get ConfigName because we haven't initialized SimulationConfig yet...
        LOG_INFO_F( "%d parameters found.\n", (EnvPtr->Config)->As<json::Object>().Size() );

        // override controller selection if unit tests requested on command line
        LOG_INFO("Initializing Controller...\n");
        IController *controller = ControllerFactory::CreateController(EnvPtr->Config);

        if (controller)
        {
            status = controller->Execute();
            if (status)
        {
            release_assert( EnvPtr );
            if ( EnvPtr->MPI.Rank == 0 )
            {
                PythonSupportPtr->RunPostProcessScript( EnvPtr->OutputPath );
            }
            LOG_INFO( "Controller executed successfully.\n" );
        }
            else
            {
                LOG_INFO( "Controller execution failed, exiting.\n" );
            }
            delete controller;
        }

#ifdef WIN32

    #ifdef DEBUG_MEMORY_LEAKS
        if (run == 1)
            _CrtMemDumpAllObjectsSince(&initial_state);
    }
    #endif


#endif

    }
    catch( Kernel::GeneralConfigurationException &e )
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << e.GetMsg() << std::endl;

        if(Kernel::JsonConfigurable::_possibleNonflatConfig && 
            Kernel::JsonConfigurable::missing_parameters_set.size() != 0)
        {
            exceptionErrorReport << "Presence of \"Default_Config_Path\" detected in config-file may indicate a problem; make sure you're using a flattened config." << std::endl;
        }
        exceptionErrorReport<< std::endl << e.GetStackTrace() << std::endl ;
    }
    catch( Kernel::DetailedException &e )
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << e.GetMsg() << std::endl << std::endl;
        exceptionErrorReport << e.GetStackTrace() << std::endl ;
    }
    catch (std::bad_alloc &e)
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << e.what() << endl;
        exceptionErrorReport << "Memory allocation failure: try reducing the memory footprint of your simulation or using more cores.\n"; 
    }
    catch (json::Exception &e)
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << "Caught json::Exception: " << e.what() << endl;

        if(Kernel::JsonConfigurable::_possibleNonflatConfig)
        {
            exceptionErrorReport << "Presence of \"Default_Config_Path\" detected in config-file may indicate a problem; make sure you're using a flattened config." << std::endl;
        }
    }
    catch (std::runtime_error &e)
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << "Caught std::runtime_error: " << e.what() << endl;
    }
    catch (std::exception &e)
    {
        exceptionErrorReport << std::endl << std::endl;
        exceptionErrorReport << e.what() <<  endl;
    } 

    // no finally in c++
    if( exceptionErrorReport.str() != "" )
    {
        LOG_ERR( exceptionErrorReport.str().c_str() );
#ifdef WIN32
        std::string msg = exceptionErrorReport.str();
        std::wstring w_msg ;
        w_msg.assign(msg.begin(), msg.end());
        OutputDebugStringW( w_msg.c_str() );
#endif
    }

    if( EnvPtr != nullptr )
    {
        EnvPtr->Log->Flush();
#if 0
        // Workaround: let's not do this.
        if((Kernel::SimulationConfig*)EnvPtr->SimConfig)
        {
            ((Kernel::SimulationConfig*)EnvPtr->SimConfig)->Release();
        }
#endif
        EnvPtr->Log->Flush();
    }

    Environment::Finalize();
    return status;
}
