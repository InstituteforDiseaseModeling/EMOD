/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>
#include <unordered_map>
#include <list>
#include <string>

#include "ISimulation.h"
#include "IReport.h"
#include "InterventionFactory.h"
#include "DllDefs.h"

#ifdef __GNUC__
#include <stdint.h>
#endif

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

#if defined(WIN32)
    bool StringEquals(const std::wstring& wStr, const char* cStr);
    bool StringEquals(const TCHAR* tStr, const char* cStr);
    wstring GetFullDllPath(std::wstring& pluginDir, const char* dllPath = nullptr);
    bool GetDllsVersion(const char* dllPath, std::wstring& wsPluginDir,list<string>& dllNames, list<string>& dllVersions);

    bool CheckEModuleVersion(HMODULE hEMod, char* emodVer=nullptr);
#endif

    string GetEModulePath(const char* emoduleDir);
    bool GetEModulesVersion(const char* dllPath, list<string>& dllNames, list<string>& dllVersions);

protected:
#if defined(WIN32)
    bool GetSimTypes( const TCHAR* pFilename, HMODULE repDll );
    bool GetReportInstantiator( const TCHAR* pFilename, 
                                HMODULE repDll, 
                                Kernel::report_instantiator_function_t* pRIF );
    bool GetType( const TCHAR* pFilename, 
                  HMODULE repDll, 
                 std::string& rClassName );
#endif
    
    bool MatchSimType(char* simTypes[]);
    bool IsValidVersion(const char* emodVer);
    void LogSimTypes(char* simTypes[]);
    void ReadEmodulesJson( const std::string& key, std::list< std::wstring > &dll_dirs );
    std::map< std::string, getSchema> getSchemaFuncPtrMap;
    std::map< std::string, std::string > dll2VersionStringMap;

private:
 
    char* m_sSimType;
    char* m_sSimTypeAll[SIMTYPES_MAXNUM];
};
