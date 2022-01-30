/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>
#include <unordered_map>
#include <list>
#include <string>

#ifdef __GNUC__
    #include <stdint.h>
    #include <errno.h>
    #include <codecvt>
    #define TCHAR wchar_t
    #define HMODULE void*
    #define GetProcAddress dlsym
#endif

#include "Sugar.h"
#include "ISimulation.h"
#include "IReport.h"
#include "InterventionFactory.h"
#include "DllDefs.h"


using namespace std;

typedef Kernel::ISimulation* (*createSim)(const Environment *);
typedef const char* (*getSchema)();

class DllLoader
{

public:

    DllLoader(const char* sSimType = nullptr);

    virtual ~DllLoader ();

    bool LoadDiseaseDlls(std::map< std::string, createSim>& createSimFuncPtrMap, const char* dllName=nullptr);
    json::Object GetDiseaseDllSchemas();
    bool LoadReportDlls( std::unordered_map< std::string, Kernel::report_instantiator_function_t >& reportInstantiators,
                         const char* dllName = nullptr );
    bool LoadInterventionDlls(const char* dllName=nullptr);

    bool StringEquals(const std::wstring& wStr, const char* cStr);
    bool StringEquals(const TCHAR* tStr, const char* cStr);
    wstring GetFullDllPath(std::wstring& pluginDir, const char* dllPath = nullptr);
    bool GetDllsVersion(const char* dllPath, std::wstring& wsPluginDir,list<string>& dllNames, list<string>& dllVersions);
    bool CheckEModuleVersion(HMODULE hEMod, char* emodVer=nullptr);

    string GetEModulePath(const char* emoduleDir);
    bool GetEModulesVersion(const char* dllPath, list<string>& dllNames, list<string>& dllVersions);

protected:
#if defined(WIN32)
    std::vector<std::wstring> GetAllMatchingDllsInDirs(std::list< std::wstring > dll_dirs, std::wstring driPlugins, const char* dllName);
    std::vector<std::wstring> GetAllMatchingDllsInDirs(std::list< std::wstring > dirsToSearch, const char* dllName=nullptr);
#else
    std::vector<std::string> GetAllMatchingSharedObjectsInDirs(std::list< std::wstring > dll_dirs, std::wstring driPlugins, const char* dllName);
    std::vector<std::string> GetAllMatchingSharedObjectsInDirs(std::list< std::wstring > dirsToSearch, const char* dllName=nullptr);
#endif
    bool CheckDynamicLibrary_Disease(void* disDll, std::string aMatchingLibrary, std::map< std::string, createSim>& createSimFuncPtrMap);
    bool CheckDynamicLibrary_Reporter(void* disDll, std::string aMatchingLibrary, std::unordered_map< std::string, Kernel::report_instantiator_function_t >& reportInstantiators);
    bool CheckDynamicLibrary_Intervention(void* disDll, std::string aMatchingLibrary);
    bool GetSimTypes( const TCHAR* pFilename, HMODULE repDll );
    bool GetReportInstantiator( const TCHAR* pFilename, 
                                HMODULE repDll, 
                                Kernel::report_instantiator_function_t* pRIF );
    bool GetType( const TCHAR* pFilename, 
                  HMODULE repDll, 
                 std::string& rClassName );
    
    bool MatchSimType(char* simTypes[]);
    bool IsValidVersion(const char* emodVer);
    void LogSimTypes(char* simTypes[]);
    void ReadEmodulesJson( const std::string& key, std::list< std::wstring > &dll_dirs );
    std::map< std::string, getSchema> getSchemaFuncPtrMap;
    std::map< std::string, std::string > dll2VersionStringMap;

private:
 
    char* m_sSimType;
    std::wstring diseasePlugins;
    std::wstring reporterPlugins;
    std::wstring interventionPlugins;
    char* m_sSimTypeAll[SIMTYPES_MAXNUM];
    std::wstring stringToWstring(std::string t_str);
    std::string DllLoader::WstringToString(std::wstring w_str);
    bool checkFileExists(const std::string& name);
};
