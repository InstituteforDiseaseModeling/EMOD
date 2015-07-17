/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <sstream> // ostringstream
#include <cstdlib>
#include <mpi.h>

#include <iostream>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include <mpi.h>
#include <iterator>

#include "Environment.h"
#include "FileSystem.h"
#include "BoostLibWrapper.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "ProgramOptions.h"
#include "Controller.h"
#include "CajunIncludes.h"
#include "Debug.h"
#include "Environment.h"
#include "ControllerFactory.h"
#include "StatusReporter.h"

#include "RANDOM.h"
#include "Exceptions.h"

// Version info extraction for both program and dlls
#include "SimulationFactory.h"
#include "ProgVersion.h"
#include "SimulationConfig.h"
#include "InterventionFactory.h"
#include "EventCoordinator.h"
#include "WaningEffect.h"
#include "CampaignEvent.h"
#include "StandardEventCoordinator.h"
#include "DllLoader.h"
#ifdef ENABLE_PYTHON
#include "Python.h"
#endif

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
    LOG_INFO_F( "Intellectual Ventures(R)/EMOD Disease Transmission Kernel %s %s %s\n", pv->getVersion(), pv->getBranch(), pv->getBuildDate() );
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


bool ControllerInitWrapper(boost::mpi::environment *mpienv, boost::mpi::communicator *world, int argc, char *argv[]); // returns false if something broke

int MPIInitWrapper( int argc, char* argv[])
{
    try 
    {
        bool fSuccessful = false;
        boost::mpi::environment env(argc, argv);
        boost::mpi::communicator world;

        try 
        {
            fSuccessful = ControllerInitWrapper(&env, &world, argc, argv);
        }
        catch( std::exception& e) 
        {
            LOG_ERR_F("Error in ControllerInitWrapper: %s\n", e.what());
        }

        // If ControllerInitWrapper() returned false (error) or an exception was
        // caught, tear everything down, decisively.
        if (!fSuccessful)
        {
            EnvPtr->Log->Flush();
            MPI_Abort(world, -1);
        }

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

#ifdef ENABLE_PYTHON

#define DEFAULT_PYTHON_HOME "c:/python27/"

#pragma warning( push )
#pragma warning( disable: 4996 )
char* PythonHomePath()
{
    char* python_path = getenv("PYTHON_PATH");
    if (!python_path)
    {
        python_path = DEFAULT_PYTHON_HOME;
    }

    std::cout << "Python home path: " << python_path << std::endl;

    return python_path;
}
#pragma warning( pop )

PyObject * 
IdmPyInit( 
    const char * python_script_name,
    const char * python_function_name
)
{
    //std::cout << __FUNCTION__ << ": " << python_script_name << ": " << python_function_name << std::endl;
    auto python_script_path = std::string("");
    if( EnvPtr != nullptr && EnvPtr->Config != nullptr && EnvPtr->Config->Exist( "Python_Script_Path") )
    {
        python_script_path = GET_CONFIG_STRING(EnvPtr->Config, "Python_Script_Path");
    }
#ifdef WIN32
    Py_SetPythonHome(PythonHomePath()); // add capability to override from command line???
#endif

    //std::cout << "Calling Py_Initialize." << std::endl;
    Py_Initialize();

    //std::cout << "Calling PySys_GetObject('path')." << std::endl;
    PyObject * sys_path = PySys_GetObject("path");

    release_assert( sys_path );
    // Get this from Environment::scripts???
    // how about we use the config.json python script path by default and if that is missing

    //std::cout << "Appending our path to existing python path." << std::endl;
    PyObject * path = nullptr;
    if( python_script_path != "" )
    {
        //std::cout << "Using dtk python path: " << GET_CONFIGURABLE(SimulationConfig)->python_script_path << std::endl;
        path = PyString_FromString( python_script_path.c_str() );
    }
    else
    {
        // because say we're running locally (or --get-schema doesn't read config.json!) we can look locally... 
        path = PyString_FromString( "." );
        //std::cout << "Using dtk python path: " << "." << std::endl;
    }
    //LOG_DEBUG_F( "Using Python Script Path: %s.\n", GET_CONFIGURABLE(SimulationConfig)->python_script_path );
    release_assert( path );
    
    //std::cout << "Calling PyList_Append." << std::endl;
    if (PyList_Append(sys_path, path) < 0)
    {
        PyErr_Print();
        throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, "Failed to append Scripts path to python path." );
    }
    // to here.

    PyObject * pName, *pModule, *pDict, *pFunc; // , *pValue;

    //std::cout << "Calling PyUnicode_FromString." << std::endl;
    pName = PyUnicode_FromString( python_script_name );
    if( !pName )
    {
        PyErr_Print();
        // Don't throw exception, return without doing anything.
        LOG_WARN( "Embedded python code failed (PyUnicode_FromString). Returning without doing anything.\n" );
        return nullptr;
    }
    //std::cout << "Calling PyImport_Import." << std::endl;
    pModule = PyImport_Import( pName );
    if( !pModule )
    {
        //PyErr_Print(); // This error message on console seems to alarm folks. We'll let python errors log as warnings.
        LOG_WARN( "Embedded python code failed (PyImport_Import). Returning without doing anything.\n" );
        return nullptr;
    }
    //std::cout << "Calling PyModule_GetDict." << std::endl;
    pDict = PyModule_GetDict( pModule ); // I don't really know what this does...
    if( !pDict )
    {
        PyErr_Print();
        throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, "Calling to PyModule_GetDict failed." );
    }
    //std::cout << "Calling PyDict_GetItemString." << std::endl;
    pFunc = PyDict_GetItemString( pDict, python_function_name ); // function name
    if( !pFunc )
    {
        PyErr_Print();
        throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, "Failed to find function 'application' in python script." );
    }
    //std::cout << "Returning from IdmPyInit." << std::endl;
    return pFunc;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// access to input schema embedding

void
postProcessSchemaFiles(
    const char* schema_path
)
{
    std::cout << __FUNCTION__ << ": " << schema_path << std::endl;
    PyObject * pFunc = IdmPyInit( "dtk_post_process_schema", "application" );
    std::cout << __FUNCTION__ << ": " << pFunc << std::endl;
    if( pFunc )
    {
        // Pass filename into python script
        PyObject * vars = PyTuple_New(1);
        PyObject* py_filename_str = PyString_FromString( schema_path );
        PyTuple_SetItem(vars, 0, py_filename_str);
        PyObject * returnArgs = PyObject_CallObject( pFunc, vars );
        std::cout << "Back from python script." << std::endl;
        PyErr_Print();
    }
    return;
}
#endif

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
    versionSchema["DTK_Branch"] = json::String( pv.getBranch() );
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

    const char * simTypeListC[] = { "GENERIC_SIM", "VECTOR_SIM", "MALARIA_SIM"
#ifdef ENABLE_POLIO
        , "POLIO_SIM"
#endif
#ifdef ENABLE_TB
        , "TB_SIM"
#endif
#ifdef ENABLE_TBHIV
        , "TBHIV_SIM"
#endif
#ifndef DISABLE_HIV
        , "STI_SIM"
        , "HIV_SIM"
#endif
    };

#define KNOWN_SIM_COUNT (sizeof(simTypeListC)/sizeof(simTypeListC[0]))

    std::vector<std::string> simTypeList( simTypeListC, simTypeListC + KNOWN_SIM_COUNT );
    json::Object configSchemaAll;
    for (auto& sim_type : simTypeList)
    {
        json::Object fakeSimTypeConfigJson;
        fakeSimTypeConfigJson["Simulation_Type"] = json::String(sim_type);
        Configuration * fakeConfig = Configuration::CopyFromElement( fakeSimTypeConfigJson );
        Kernel::SimulationConfig * pConfig = Kernel::SimulationConfigFactory::CreateInstance(fakeConfig);
        release_assert( pConfig );

        json::QuickBuilder config_schema = pConfig->GetSchema();
        configSchemaAll[sim_type] = config_schema;
    }

    for (auto& entry : Kernel::JsonConfigurable::get_registration_map())
    {
        const std::string& classname = entry.first;
        json::QuickBuilder config_schema = ((*(entry.second))());
        configSchemaAll[classname] = config_schema;
    }

    total_schema[ "config" ] = configSchemaAll;

    json::Object fakeJsonRoot3;
    json::QuickBuilder fakeECJson( fakeJsonRoot3 );
    fakeECJson["class"] = json::String("StandardInterventionDistributionEventCoordinator");
    auto fakeConfig = Configuration::CopyFromElement( fakeECJson );
    Kernel::StandardInterventionDistributionEventCoordinator * pTempEC = dynamic_cast<Kernel::StandardInterventionDistributionEventCoordinator*>( Kernel::EventCoordinatorFactory::CreateInstance( fakeConfig ) );
    json::QuickBuilder ec_schema = pTempEC->GetSchema();

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

#ifdef ENABLE_PYTHON
    if( szOutputPath != "stdout" )
    {
        std::cout << "Successfully created schema in file " << output_path << ". Attempting to post-process." << std::endl;
        postProcessSchemaFiles( output_path );
    }
#endif
}

void
pythonOnExitHook()
{
    //PyObject * pName, *pModule, *pDict, *pFunc, *pValue;

#ifdef ENABLE_PYTHON
    auto pFunc = IdmPyInit( "dtk_post_process", "application" );
    if( pFunc )
    {
        PyObject * vars = NULL;
        PyObject_CallObject( pFunc, vars );
        PyErr_Print();
    }
#endif
}

bool ControllerInitWrapper(boost::mpi::environment *mpienv, boost::mpi::communicator *world, int argc, char *argv[])
{
    using namespace std;

    /////////////////////////////////////////////////////////////
    // parse input arguments

    ProgramOptions po("Recognized options");
    try
    {
        po.AddOption( "help",         "Show this help message." );
        po.AddOption( "version", "v", "Get version info." );
        po.AddOption("get-schema",     "Request the kernel to write all its input definition schema json to the current working directory and exit." );
        po.AddOptionWithValue( "schema-path", "stdout",                        "Path to write schema(s) to instead of writing to stdout." );
        po.AddOptionWithValue( "config",       "C",          "default-config.json", "Name of config.json file to use" );
        po.AddOptionWithValue( "input-path",   "I",          ".",                   "Relative or absolute path to location of model input files" );       
        po.AddOptionWithValue( "output-path",  "O",          "output",              "Relative or absolute path for output files" );
        po.AddOptionWithValue( "dll-path",     "D",          "",                    "Relative (to the executable) or absolute path for EMODule (dll/shared object) root directory" );
// 2.5        po.AddOptionWithValue( "state-path",   "S",          ".",                   "Relative or absolute path for state files" );  // should we remove this...?
        po.AddOptionWithValue( "monitor_host",               "none",                "IP of commissioning/monitoring host" );
        po.AddOptionWithValue( "monitor_port",               0,                     "port of commissioning/monitoring host" );
        po.AddOptionWithValue( "sim_id",                    "none",                 "Unique id of this simulation, formerly sim_guid. Needed for self-identification to UDP host" );
        po.AddOption( "progress",        "Send updates on the progress of the simulation to the HPC job scheduler." );


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
                        iter1++, iter2++)
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

    // --------------------------------------------------------------------------------------------------
    // --- DMB 9-9-2014 ERAD-1621 Users complained that a file not found exception on default-config.json
    // --- really didn't tell them that they forgot to specify the configuration file on the command line.
    // --- One should be able to get the schema without specifying a config file.
    // --------------------------------------------------------------------------------------------------
    if( (po.GetCommandLineValueString( "config" ) == po.GetCommandLineValueDefaultString( "config" )) &&
       !po.CommandLineHas( "get-schema" ) )
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

        // set up the environment
        EnvPtr->Log->Flush();
        LOG_INFO("Initializing environment...\n");
        bool env_ok = Environment::Initialize(
            mpienv,
            world,
            po.GetCommandLineValueString( "config" ),
            po.GetCommandLineValueString( "input-path"  ),
            po.GetCommandLineValueString( "output-path" ),
// 2.5            po.GetCommandLineValueString( "state-path"  ),
            po.GetCommandLineValueString( "dll-path"    ),
            po.CommandLineHas( "get-schema" )
            );

        if (!env_ok)
        {
            LOG_ERR("Failed to initialize environment, exiting\n");
            return false;
        }

        if( po.CommandLineHas( "get-schema" ) )
        {
            writeInputSchemas( po.GetCommandLineValueString( "dll-path" ).c_str(), po.GetCommandLineValueString( "schema-path" ).c_str() );
            return true;
        }

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

#if 0
    Kernel::SimulationConfig* SimConfig = Kernel::SimulationConfigFactory::CreateInstance(EnvPtr->Config);
    if (SimConfig)
    {
        Environment::setSimulationConfig(SimConfig);
        LOG_INFO_F( "setSimulationConfig: %p\n", SimConfig );
    }
    else
    {
        LOG_ERR("Failed to create SimulationConfig instance\n");
        return false;
    }
#endif

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

        IController *controller = NULL;

        // override controller selection if unit tests requested on command line
        LOG_INFO("Initializing Controller...\n");
        controller = ControllerFactory::CreateController(EnvPtr->Config);
            
        if (controller)
        {
            status = controller->Execute();
            if (status)
            {
                pythonOnExitHook();
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

    if( EnvPtr != NULL )
    {
        EnvPtr->Log->Flush();
        if((Kernel::SimulationConfig*)EnvPtr->SimConfig)
        {
            ((Kernel::SimulationConfig*)EnvPtr->SimConfig)->Release();
        }
    }
    EnvPtr->Log->Flush();

    Environment::Finalize();
    return status;
}
