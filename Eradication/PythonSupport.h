/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#pragma warning(disable : 4996)

#include <string>

#include "Environment.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#ifdef ENABLE_PYTHON

// This disgusting set of macros is so that we can build in Debug without a linker error on python27_d.lib.
// Python.h includes pyconfig.h which forces the use of python27_d.lib if _DEBUG is defined.  The following
// macros are to force python to use python27.lib.
#ifdef WIN32
#ifdef _DEBUG
#undef _DEBUG
#endif
#endif

#include "Python.h"

// This assumes that NDEBUG is defined for Release and
// _DEBUG is only defined for Debug.
#ifdef WIN32
#ifndef NDEBUG
#define _DEBUG
#endif
#endif

#endif //ENABLE_PYTHON
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


#define PythonSupportPtr static_cast<Kernel::PythonSupport*>(EnvPtr->pPythonSupport)

namespace Kernel
{
    class PythonSupport
    {
    public:
        static std::string SCRIPT_PRE_PROCESS;
        static std::string SCRIPT_POST_PROCESS;
        static std::string SCRIPT_POST_PROCESS_SCHEMA;
        static std::string SCRIPT_PYTHON_FEVER;

        PythonSupport();
        ~PythonSupport();

        void CheckPythonSupport( bool isGettingSchema, const std::string& pythonScriptPath );

#ifdef ENABLE_PYTHON
        PyObject* IdmPyInit( const char * python_script_name, const char * python_function_name );
#endif

        std::string RunPreProcessScript( const std::string& configFileName );
        void RunPostProcessScript( const std::string& outputPath );
        void RunPostProcessSchemaScript( const std::string& schemaPath );
        void CheckSimScripts( const std::string& simTypeStr );

    private:
        std::string CreatePythonScriptPath( const std::string& script_filename );
        bool PythonScriptCheckExists( const std::string&script_filename );
        void PythonScriptsNotFound();
        std::string PythonHomePath();

        std::string m_PythonScriptPath;
        bool m_IsGettingSchema;
        bool m_CheckForSimScripts;
    };
}