/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "PythonSupport.h"
#include "Exceptions.h"
#include "FileSystem.h"
#include "Debug.h"
#include "Log.h"

#define DEFAULT_PYTHON_HOME         "c:/Python36"
#define ENV_VAR_PYTHON              "IDM_PYTHON3_PATH"
#define PYTHON_DLL_W               L"python36.dll"
#define PYTHON_DLL_S                "python36.dll"

#define PYTHON_SCRIPT_PATH_NOT_SET ""

SETUP_LOGGING("PythonSupport")

namespace Kernel
{
    std::string PythonSupport::FUNCTION_NAME              = "application";
    std::string PythonSupport::SCRIPT_PRE_PROCESS         = "dtk_pre_process";
    std::string PythonSupport::SCRIPT_POST_PROCESS        = "dtk_post_process";
    std::string PythonSupport::SCRIPT_POST_PROCESS_SCHEMA = "dtk_post_process_schema";

    std::string PythonSupport::SCRIPT_PYTHON_FEVER        = "dtk_pydemo_individual";
    std::string PythonSupport::SCRIPT_TYPHOID             = "dtk_typhoid_individual";
    bool PythonSupport::m_PythonInitialized               = false; 
    std::string PythonSupport::m_PythonScriptPath         = PYTHON_SCRIPT_PATH_NOT_SET;

    PythonSupport::PythonSupport()
    {
    }

    PythonSupport::~PythonSupport()
    {
        cleanPython();
    }

    bool PythonSupport::IsPythonInitialized()
    {
        return m_PythonInitialized;
    }

    bool PythonSupport::PythonScriptsExist()
    {
         // Simple business-logic check to see if python initialized but no scripts.
         bool fcheck1 = FileSystem::FileExistsInPath(                ".", std::string( SCRIPT_PRE_PROCESS )  + ".py" );
         bool fcheck2 = FileSystem::FileExistsInPath(                ".", std::string( SCRIPT_POST_PROCESS ) + ".py" );
         bool fcheck3 = FileSystem::FileExistsInPath(                ".", std::string( SCRIPT_PYTHON_FEVER ) + ".py" );
         bool fcheck4 = FileSystem::FileExistsInPath( m_PythonScriptPath, std::string( SCRIPT_PRE_PROCESS )  + ".py" );
         bool fcheck5 = FileSystem::FileExistsInPath( m_PythonScriptPath, std::string( SCRIPT_POST_PROCESS ) + ".py" );

        return ( fcheck1 || fcheck2 || fcheck3 || fcheck4 || fcheck5 );
    }

    void PythonSupport::SetupPython( const std::string& pythonScriptPath )
    {
        m_PythonScriptPath = pythonScriptPath;

#ifdef ENABLE_PYTHON
        if( m_PythonScriptPath == PYTHON_SCRIPT_PATH_NOT_SET )
        {
            LOG_INFO( "Python not initialized because --python-script-path (-P) not set.\n" );
            return;
        }
        else
        {
            LOG_INFO_F( "Python script path: %s\n", m_PythonScriptPath.c_str() );
        }
#ifdef WIN32
        HMODULE p_dll = LoadLibrary( PYTHON_DLL_W );
        if( p_dll == nullptr )
        {
            std::stringstream msg;
            msg << "Cannot run python scripts because " << PYTHON_DLL_S << " cannot be found.";
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        // Get path to python installation
        char* c_python_path = getenv(ENV_VAR_PYTHON);
        if( c_python_path == nullptr )
        {
            std::stringstream msg;
            msg << "Cannot find environmental variable " << ENV_VAR_PYTHON << ".\n"
                << "Assuming default path for python installation.";
            LOG_INFO( msg.str().c_str() );
            c_python_path = DEFAULT_PYTHON_HOME;
        }
        std::string python_home = c_python_path;
        python_home = FileSystem::RemoveTrailingChars( python_home );
        LOG_INFO_F( "Python home path: %s\n", python_home.c_str() );

        if( !FileSystem::DirectoryExists( python_home ) )
        {
            std::stringstream msg;
            msg << PYTHON_DLL_S << " was found but IDM_PYTHON3_PATH=" << python_home 
                << " was not found.  Default is " << DEFAULT_PYTHON_HOME;
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        Py_SetPythonHome( Py_DecodeLocale( python_home.c_str(), nullptr ) );
#endif

        // Open a python instance; remember to cleanPython before throwing an exception
        Py_Initialize();
        m_PythonInitialized = true;

        // Prepend python-script-path and current working directory to current python instance.
        PyObject* sys_path = PySys_GetObject("path");
        release_assert( sys_path );

        PyObject* path_user = PyUnicode_FromString( m_PythonScriptPath.c_str() );
        release_assert( path_user );
        release_assert( !PyList_Insert(sys_path, 0, path_user) );
        
        PyObject* path_default = PyUnicode_FromString( "" );
        release_assert( path_default );
        release_assert( !PyList_Insert(sys_path, 0, path_default) );
#endif
        return;
    }

    void PythonSupport::cleanPython()
    {
#ifdef ENABLE_PYTHON
        // Close python instance (flush stdout/stderr)
        if( m_PythonInitialized )
        {
            if(PyErr_Occurred())
            {
                PyErr_Print();
            }
            Py_FinalizeEx();
            m_PythonInitialized = false;
        }
        // Best practice is to open (Py_Initialize) and close (Py_FinalizeEx) the interpreter 
        // around every script execution. Unfortunately, some dynamically imported modules
        // (see numpy/issues/8097) aren't cleaned up properly, and importing a second time 
        // after a restart will crash everything. Instead, Py_Initialize is invoked once during
        // SetupPython and Py_FinalizeEx is only invoked in cleanPython. 
#endif
        return;
    }

    std::string PythonSupport::RunPyFunction( const std::string& arg_string, const std::string& python_module, const std::string& python_function )
    {
        std::string returnString = arg_string;

        // Return immediately if no python or script doesn't exist
        if( !m_PythonInitialized )
        {
            return returnString;
        }

        LOG_DEBUG_F( "Checking current working directory and python script path for embedded python script %s.\n", python_module.c_str() );
        // Check for script on two paths: python-script-path and current directory
        if( !FileSystem::FileExistsInPath( ".", std::string(python_module)+".py" ) &&
            !FileSystem::FileExistsInPath( m_PythonScriptPath, std::string(python_module)+".py" ) )
        {
            LOG_DEBUG("File not found.\n");
            return returnString;
        }

#ifdef ENABLE_PYTHON
        // Flush output in preparation for processing
        EnvPtr->Log->Flush();

        // Get function pointer from script
        PyObject* pFunc = GetPyFunction( python_module.c_str(), python_function.c_str() );

        // Initialize function arguments
        PyObject* vars  = PyTuple_New(1);
        release_assert( vars );
        PyObject* py_argstring = PyUnicode_FromString( arg_string.c_str() );
        release_assert( py_argstring );
        if (PyTuple_SetItem(vars, 0, py_argstring))
        {
            std::stringstream msg;
            msg << "Failed to construct arguments to python function.";
            cleanPython();
            throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        // Call python function with arguments
        PyObject* retValue = PyObject_CallObject( pFunc, vars );
        if(!retValue)
        {
            std::stringstream msg;
            msg << "Python function '" << python_function << "' in " << python_module << ".py failed.";
            cleanPython();
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        // Handle return value
        if(PyUnicode_Check(retValue))
        {
            char* retValuePtr = PyUnicode_AsUTF8(retValue);
            release_assert( retValuePtr );
            returnString.assign( retValuePtr );
        }
        else if (retValue != Py_BuildValue(""))
        {
            std::stringstream msg;
            msg << "Python function '" << python_function << "' in " << python_module
                << ".py did not provide an expected return value. Expected: string or None.";
            cleanPython();
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        // Ensure output
        PyObject* sys_stdout = PySys_GetObject("stdout");
        release_assert( sys_stdout );
        release_assert(PyObject_CallMethod(sys_stdout, "flush", nullptr));
#endif
        return returnString;
    }

#ifdef ENABLE_PYTHON
    PyObject* PythonSupport::GetPyFunction( const char * python_module, const char * python_function )
    {
        // Loads contents of script as a module into current python instance
        PyObject* pName = PyUnicode_FromString( python_module );
        release_assert( pName );
        PyObject* pModule = PyImport_Import( pName );
        if(PyErr_Occurred())
        {
            std::stringstream msg;
            msg << "Python script '" << python_module << "' failed to import as module.";
            cleanPython();
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        // Verifies existance of function in module
        PyObject* pDict = PyModule_GetDict( pModule );
        release_assert( pDict );
        PyObject* pFunc = PyDict_GetItemString( pDict, python_function );
        if(!pFunc)
        {
            std::stringstream msg;
            msg << "Python module " << python_module << " does not contain function '"
                << python_function << "'.";
            cleanPython();
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        // Return pointer to an unverified python function; can't verify here with a delay-load dll
        return pFunc;
    }
#endif
}
