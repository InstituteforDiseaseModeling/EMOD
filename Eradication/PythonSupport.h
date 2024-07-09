
#pragma once

#pragma warning(disable : 4996) // wcstombs function is deprecated on windows

#include <string>
#include <map>
#include <vector>


// This set of macros prevents the Python.h header from trying to use the _d.dll version of 
// the library when building debug; assumes NDEBUG only defined for Release builds
#ifdef ENABLE_PYTHON
#ifdef WIN32
#ifdef _DEBUG
#undef _DEBUG
#endif
#endif

// Stable Application Binary Interface (ABI):
//
//   API compatibility does not extend to binary compatibility (the ABI). As a consequence,
//   extension modules need to be recompiled for every Python release. In addition, on
//   Windows, extension modules link with a specific pythonXY.dll and need to be recompiled
//   to link with a newer one.
//
//   A subset of the API has been declared to guarantee a stable ABI. Extension modules using
//   this API (limited API) need to define Py_LIMITED_API. Many interpreter details are hidden
//   from the extension module; in return, the module works on any 3.x version (x>=2) without
//   recompilation.
//
//   In some cases, the stable ABI needs to be extended with new functions. Extension modules
//   wishing to use these new APIs need to set Py_LIMITED_API to the PY_VERSION_HEX value of
//   the minimum Python version they want to support (e.g. 0x03030000 for Python 3.3). Such
//   modules will work on all subsequent Python releases, but fail to load on older releases.

#define PY_SSIZE_T_CLEAN
#define Py_LIMITED_API 0x03060000
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
        static std::string SCRIPT_PRE_PROCESS;
        static std::string SCRIPT_POST_PROCESS;
        static std::string SCRIPT_POST_PROCESS_SCHEMA;
        static std::string SCRIPT_IN_PROCESS;
        static std::string SCRIPT_VECTOR_SURVEILLANCE;

        static std::string FUNCTION_NAME;

        static std::map<std::string, std::map<std::string, void*>> PyObjectsMap;

        PythonSupport();
        ~PythonSupport();

        static bool          IsPythonInitialized();
        static void          SetupPython( const std::string& python_script_paths );
        static std::string   RunPyFunction( const std::string& arg_string, const std::string& python_module_name, const std::string& python_function_name = FUNCTION_NAME );

        static bool          ImportPyModule( const std::string& python_module_name );

        static void*         GetPyFunction( const std::string& python_module_name, const std::string& python_function_name );

    private:
        static void         cleanPython();

        static bool                      m_PythonInitialized;
        static std::vector<std::string>  m_python_paths;
    };
}
