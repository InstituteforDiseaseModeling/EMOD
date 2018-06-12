/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#pragma warning(disable : 4996) // getenv function is deprecated

#include <string>

#include "Environment.h"


// This set of macros prevents the Python.h header from trying to use the _d.dll version of 
// the library when building debug; assumes NDEBUG only defined for Release builds
#ifdef ENABLE_PYTHON
#ifdef WIN32
#ifdef _DEBUG
#undef _DEBUG
#endif
#endif

#include "Python.h"

#ifdef WIN32
#ifndef NDEBUG
#define _DEBUG
#endif
#endif
#endif


namespace Kernel
{
    class PythonSupport
    {
    public:
        static std::string FUNCTION_NAME;
        static std::string SCRIPT_PRE_PROCESS;
        static std::string SCRIPT_POST_PROCESS;
        static std::string SCRIPT_POST_PROCESS_SCHEMA;

        static std::string SCRIPT_PYTHON_FEVER;
        static std::string SCRIPT_TYPHOID;

        PythonSupport();
        ~PythonSupport();

        static bool          IsPythonInitialized();
        static bool          PythonScriptsExist();
        static void          SetupPython( const std::string& pythonScriptPath );
        static std::string   RunPyFunction( const std::string& arg_string, const std::string& python_module, const std::string& python_function = FUNCTION_NAME );

#ifdef ENABLE_PYTHON
        static PyObject*     GetPyFunction( const char * python_module, const char * python_function );
#endif

    private:
        static void         cleanPython();

        static bool         m_PythonInitialized;
        static std::string  m_PythonScriptPath;
    };
}
