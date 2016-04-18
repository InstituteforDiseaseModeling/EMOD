/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "PythonSupport.h"
#include "Exceptions.h"
#include "FileSystem.h"
#include "Debug.h"

#define DEFAULT_PYTHON_HOME "c:/Python27"
#define PYTHON_DLL_W          L"python27.dll"
#define PYTHON_DLL_S           "python27.dll"

#define PYTHON_SCRIPT_PATH_NOT_SET ""
 
namespace Kernel
{
    std::string PythonSupport::SCRIPT_PRE_PROCESS         = "dtk_pre_process";
    std::string PythonSupport::SCRIPT_POST_PROCESS        = "dtk_post_process";
    std::string PythonSupport::SCRIPT_POST_PROCESS_SCHEMA = "dtk_post_process_schema";
    std::string PythonSupport::SCRIPT_PYTHON_FEVER        = "dtk_pydemo_individual";


    PythonSupport::PythonSupport()
    : m_PythonScriptPath(PYTHON_SCRIPT_PATH_NOT_SET)
    , m_IsGettingSchema(false)
    , m_CheckForSimScripts(false)
    {
    }

    PythonSupport::~PythonSupport()
    {
    }

#pragma warning( push )
#pragma warning( disable: 4996 )
    std::string PythonSupport::PythonHomePath()
    {
        char* c_python_path = getenv("PYTHONHOME");
        if( c_python_path == nullptr )
        {
            c_python_path = DEFAULT_PYTHON_HOME;
        }
        std::string str_python_path = c_python_path;
        str_python_path = FileSystem::RemoveTrailingChars( str_python_path );
        
        std::cout << "Python home path: " << str_python_path << std::endl;

        return str_python_path;
    }
#pragma warning( pop )

    std::string PythonSupport::CreatePythonScriptPath( const std::string& script_filename )
    {
        std::string path_to_script = FileSystem::Concat( m_PythonScriptPath, std::string(script_filename)+".py" );
        return path_to_script;
    }

    bool PythonSupport::PythonScriptCheckExists( const std::string& script_filename )
    {
        bool exists = FileSystem::FileExists( CreatePythonScriptPath( script_filename ) );
        return exists;
    }

    void PythonSupport::PythonScriptsNotFound()
    {
        std::string script_pre  = CreatePythonScriptPath( SCRIPT_PRE_PROCESS  );
        std::string script_post = CreatePythonScriptPath( SCRIPT_POST_PROCESS );
        std::stringstream msg;
        msg << "The --python-script-path command line option was given but the pre (" << script_pre << ") ";
        msg << "and post (" << script_post << ") processing scripts cannot be found";
        throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
    }

    void PythonSupport::CheckPythonSupport( bool isGettingSchema, const std::string& pythonScriptPath )
    {
        m_IsGettingSchema = isGettingSchema;
        m_PythonScriptPath = pythonScriptPath;

#ifdef ENABLE_PYTHON
        if( m_PythonScriptPath == PYTHON_SCRIPT_PATH_NOT_SET )
        {
            return;
        }
#ifdef WIN32
        HMODULE p_dll = LoadLibrary( PYTHON_DLL_W );
        if( p_dll == nullptr )
        {
            std::stringstream msg;
            msg << "Cannot run python scripts because " << PYTHON_DLL_S << " cannot be found.";
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        if( m_IsGettingSchema )
        {
            if( !PythonScriptCheckExists( SCRIPT_POST_PROCESS_SCHEMA ) )
            {
                std::string path_to_script = CreatePythonScriptPath( SCRIPT_POST_PROCESS_SCHEMA );

                std::stringstream msg;
                msg << "The --get-schema and --python-script-path command line options were provided but cannot find " << path_to_script;
                throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
        else
        {
            // ------------------------------------------------------------------
            // --- None of the scripts can be found.  After reading the config,
            // --- check if there are other simulation based scripts that might be used
            // ------------------------------------------------------------------
            m_CheckForSimScripts =  !PythonScriptCheckExists( SCRIPT_PRE_PROCESS  ) 
                                 && !PythonScriptCheckExists( SCRIPT_POST_PROCESS );
        }
#endif
#endif
    }

#ifdef ENABLE_PYTHON
    PyObject* PythonSupport::IdmPyInit( const char * python_script_name, const char * python_function_name )
    {
        if( m_PythonScriptPath == PYTHON_SCRIPT_PATH_NOT_SET )
        {
            return nullptr;
        }

        if( !PythonScriptCheckExists( python_script_name ) )
        {
            return nullptr;
        }

#ifdef WIN32
        std::string python_home = PythonHomePath();
        if( !FileSystem::DirectoryExists( python_home ) )
        {
            std::stringstream msg;
            msg << PYTHON_DLL_S << " was found but PYTHONHOME=" << python_home << " was not found.  Default is " << DEFAULT_PYTHON_HOME;
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        Py_SetPythonHome( const_cast<char*>(python_home.c_str()) ); // add capability to override from command line???
#endif

        Py_Initialize();

        //std::cout << "Calling PySys_GetObject('path')." << std::endl;
        PyObject * sys_path = PySys_GetObject("path");

        release_assert( sys_path );
        // Get this from Environment::scripts???
        // how about we use the config.json python script path by default and if that is missing

        //std::cout << "Appending our path to existing python path." << std::endl;
        if( m_PythonScriptPath == PYTHON_SCRIPT_PATH_NOT_SET )
        {
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "python-script-path must be set." );
        }
        std::cout << "Using (configured) python_script_path: " << m_PythonScriptPath << std::endl;
        PyObject* path = PyString_FromString( m_PythonScriptPath.c_str() );
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

            std::stringstream msg;
            msg << "Embedded python code failed (PyUnicode_FromString) to load " << python_script_name;
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        //std::cout << "Calling PyImport_Import." << std::endl;
        pModule = PyImport_Import( pName );
        if( !pModule )
        {
            PyErr_Print(); // This error message on console seems to alarm folks. We'll let python errors log as warnings.

            std::stringstream msg;
            msg << "Embedded python code failed (PyImport_Import) to load " << python_script_name;
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
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
            std::stringstream msg;
            msg << "Failed to find function '" << python_function_name << "' in python script.";
            throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str());
        }
        //std::cout << "Returning from IdmPyInit." << std::endl;
        return pFunc;
    }
#endif // ENABLE_PYTHON

    std::string PythonSupport::RunPreProcessScript( const std::string& configFileName )
    {
        std::string return_filename = configFileName;
#ifdef ENABLE_PYTHON
        if( !m_IsGettingSchema )
        {
            auto pFunc = IdmPyInit( SCRIPT_PRE_PROCESS.c_str(), "application" );
            if( pFunc )
            {
                PyObject * vars = PyTuple_New(1);
                PyObject* py_filename_str = PyString_FromString( configFileName.c_str() );
                PyTuple_SetItem(vars, 0, py_filename_str);
                auto retValue = PyObject_CallObject( pFunc, vars );
                PyErr_Print();
                if( retValue != nullptr && std::string( retValue->ob_type->tp_name ) != "NoneType" )
                {
                    return_filename = PyString_AsString( retValue );
                }
                else
                {
                    std::stringstream msg;
                    msg << "'application' function in python pre-process script failed to return string.";
                    throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str());
                }
            }
        }
#endif
        return return_filename;
    }

    void PythonSupport::RunPostProcessScript( const std::string& outputPath )
    {
#ifdef ENABLE_PYTHON
        auto pFunc = IdmPyInit( SCRIPT_POST_PROCESS.c_str(), "application" );
        if( pFunc )
        {
            PyObject* vars = PyTuple_New(1);
            PyObject* py_oppath_str = PyString_FromString( outputPath.c_str() );
            PyTuple_SetItem(vars, 0, py_oppath_str );
            PyObject* returnArgs = PyObject_CallObject( pFunc, vars );
            PyErr_Print();
        }
#endif
    }

    void PythonSupport::RunPostProcessSchemaScript( const std::string& schemaPath )
    {
#ifdef ENABLE_PYTHON
        std::cout << "Successfully created schema in file " << schemaPath << ". Attempting to post-process." << std::endl;

        //std::cout << __FUNCTION__ << ": " << schema_path << std::endl;
        PyObject * pFunc = IdmPyInit( SCRIPT_POST_PROCESS_SCHEMA.c_str(), "application" );
        //std::cout << __FUNCTION__ << ": " << pFunc << std::endl;
        if( pFunc )
        {
            // Pass filename into python script
            PyObject * vars = PyTuple_New(1);
            PyObject* py_filename_str = PyString_FromString( schemaPath.c_str() );
            PyTuple_SetItem(vars, 0, py_filename_str);
            /* PyObject * returnArgs = */ PyObject_CallObject( pFunc, vars );
            //std::cout << "Back from python script." << std::endl;
            PyErr_Print();
        }
#endif
    }

    void PythonSupport::CheckSimScripts( const std::string& simTypeStr )
    {
#ifdef ENABLE_PYTHON
        if( m_CheckForSimScripts )
        {
#ifdef ENABLE_PYTHON_FEVER
            if( simTypeStr == "PY_SIM" )
            {
                if( !PythonScriptCheckExists( SCRIPT_PYTHON_FEVER ) )
                {
                    std::string path_to_script = CreatePythonScriptPath( SCRIPT_PYTHON_FEVER );
                    std::stringstream msg;
                    msg << "Simulation_Type=PY_SIM but " << path_to_script << " cannot be found.";
                    throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
            }
            else
            {
                PythonScriptsNotFound();
            }
#else
            PythonScriptsNotFound();
#endif
        }
#endif
    }
}
