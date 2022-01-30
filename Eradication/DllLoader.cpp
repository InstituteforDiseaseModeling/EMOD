/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// Dll export and loading platform implementation
//
#include "stdafx.h"

#include <string.h>
#include <iostream>
#include "DllLoader.h"
#include "Debug.h"
#include "Environment.h"
#include "FileSystem.h"
#include "Log.h"
#include "ProgVersion.h"
#include "Exceptions.h"
#include "CajunIncludes.h"
#ifdef __GNUC__
#include <dlfcn.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#endif

typedef char* (*gver)(char*,const Environment * pEnv);

SETUP_LOGGING( "DllLoader" )

#pragma warning(disable : 4996)

/////////////////////////////////////////////////////////////
// Important Note:
// GetEModuleVersion has to be the first exported function to call
// to make sure the environment instance is set from main exe.
/////////////////////////////////////////////////////////////////

DllLoader::DllLoader(const char* sSimType)
{
    m_sSimType = nullptr;
    memset(m_sSimTypeAll, 0, SIMTYPES_MAXNUM*sizeof(char *));
    if (sSimType)
    {
        m_sSimType = new char[strlen(sSimType)+1];
        strcpy(m_sSimType, sSimType);
    }

#ifdef WIN32
    this->diseasePlugins = std::wstring(DISEASE_PLUGINS);
    this->reporterPlugins = std::wstring(REPORTER_PLUGINS);
    this->interventionPlugins = std::wstring(INTERVENTION_PLUGINS);
#else
    this->diseasePlugins = stringToWstring(DISEASE_PLUGINS);
    this->reporterPlugins = stringToWstring(REPORTER_PLUGINS);
    this->interventionPlugins = stringToWstring(INTERVENTION_PLUGINS);
#endif
}

DllLoader::~DllLoader ()
{
    if (m_sSimType)
    {
        delete m_sSimType;
        m_sSimType = nullptr;
    }
    int i=0;
    while (m_sSimTypeAll[i] != nullptr && i<SIMTYPES_MAXNUM )
    {
        delete m_sSimTypeAll[i];
        m_sSimTypeAll[i] = nullptr;
        i++;
    }
}

void
DllLoader::ReadEmodulesJson(
    const std::string& key,
    std::list< std::wstring > &dll_dirs
)
{
    if( !FileSystem::FileExists( "emodules_map.json" ) )
    {
        // actually, might be ok if just using --dll_dir, not settled yet
        LOG_INFO( "ReadEmodulesJson: no file, returning.\n" );
        return;
    }

    LOG_DEBUG( "open emodules_map.json\n" );
    std::ifstream emodules_json_raw;
    FileSystem::OpenFileForReading( emodules_json_raw, "emodules_map.json" );

    try
    {
        json::Object emodules;
        json::Reader::Read(emodules, emodules_json_raw);
        const json::Array &disease_dll_array = json::json_cast<const json::Array&>( emodules[ key ] );
        for( unsigned int idx = 0; idx < disease_dll_array.Size(); ++idx )
        {
            LOG_DEBUG_F( "Getting dll_path at idx %d\n", idx );
            const std::string &dll_path = json::json_cast<const json::String&>(disease_dll_array[idx]);
            LOG_DEBUG_F( "Found dll_path %s\n", dll_path.c_str() );

            std::wstring str2(std::string( dll_path ).length(), L' ');
            std::copy(dll_path.begin(), dll_path.end(), str2.begin());
            dll_dirs.push_back( str2 );

            LOG_INFO_F( "Stored dll_path %S as wstring in list.\n", str2.c_str() );
        }
        LOG_INFO( "Stored all dll_paths.\n" );
    }
    catch( const json::Exception &e )
    {
        throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, e.what() );
    }
    emodules_json_raw.close();
}

bool
DllLoader::LoadDiseaseDlls(
    std::map< std::string, createSim>& createSimFuncPtrMap,
    const char* dllName
)
{
    LOG_DEBUG("Enter LoadDiseaseDlls\n");
    bool bRet = false;
    if (!EnvPtr)
    {
        LOG_ERR("LoadDiseaseDlls: EnvPtr not initialized. Returning.");
        return bRet;
    }

    std::list< std::wstring > disease_dll_dirs;
    ReadEmodulesJson( DISEASE_EMODULES, disease_dll_dirs );

#if defined(WIN32)
    std::vector< std::wstring> allMatchingDLLs = GetAllMatchingDllsInDirs( disease_dll_dirs, diseasePlugins, dllName );
    for (std::wstring aMatchingDll : allMatchingDLLs)
    {
        LOG_INFO_F("Calling LoadLibrary for %S\n", aMatchingDll.c_str());
        HMODULE disDll = LoadLibrary( aMatchingDll.c_str() );
        
        if( CheckDynamicLibrary_Disease( disDll, wstringToString(aMatchingDll), createSimFuncPtrMap ) == true )
        {
            bRet = true;
        }
    }
#else
    std::vector<std::string> allMatchingSharedObjs = GetAllMatchingSharedObjectsInDirs( disease_dll_dirs, diseasePlugins, dllName );
    void* disDll = nullptr;
    for (std::string aMatchingSharedObject : allMatchingSharedObjs)
    {
        LOG_INFO_F("Loading Library: %S\n", aMatchingSharedObject.c_str());
        disDll = dlopen( aMatchingSharedObject.c_str(), RTLD_NOW );
        
        if( CheckDynamicLibrary_Disease( disDll, aMatchingSharedObject, createSimFuncPtrMap ) == true )
        {
            bRet = true;
        }
    }
#endif

    return bRet;
}

bool DllLoader::LoadReportDlls( std::unordered_map< std::string, Kernel::report_instantiator_function_t >& reportInstantiators,
                                const char* dllName)
{
    LOG_DEBUG("Enter LoadReportDlls\n");
    bool bRet = false;
    if (!EnvPtr)
    {
        LOG_ERR("LoadReportDlls: EnvPtr not initialized. Returning.");
        return bRet;
    }

    std::list< std::wstring > report_dll_dirs;
    ReadEmodulesJson( REPORTER_EMODULES, report_dll_dirs );


#if defined(WIN32)
    std::vector< std::wstring> allMatchingDLLs = GetAllMatchingDllsInDirs( report_dll_dirs, reporterPlugins, dllName );
    for (std::wstring aMatchingDll : allMatchingDLLs)
    {
        LOG_INFO_F("Calling LoadLibrary for %S\n", aMatchingDll.c_str());
        HMODULE disDll = LoadLibrary( aMatchingDll.c_str() );
        
        if( CheckDynamicLibrary_Reporter( disDll, wstringToString(aMatchingDll), reportInstantiators ) == true )
        {
            bRet = true;
        }
    }
#else
    std::vector<std::string> allMatchingSharedObjs = GetAllMatchingSharedObjectsInDirs( report_dll_dirs, reporterPlugins, dllName );
    void* disDll = nullptr;
    for (std::string aMatchingSharedObject : allMatchingSharedObjs)
    {
        LOG_INFO_F("Loading Library: %S\n", aMatchingSharedObject.c_str());
        disDll = dlopen( aMatchingSharedObject.c_str(), RTLD_NOW );
        
        if( CheckDynamicLibrary_Reporter( disDll, aMatchingSharedObject, reportInstantiators ) == true )
        {
            bRet = true;
        }
    }
#endif

    return bRet;
}


bool DllLoader::GetSimTypes( const TCHAR* pFilename, HMODULE repDll )
{
    bool success = true ;
    LOG_INFO_F( "Calling GetProcAddress for GetSupportedSimTypes on %S\n", pFilename );
    typedef void (*gst)(char* simType[]);
    gst _gst = (gst)GetProcAddress( repDll, "GetSupportedSimTypes" );
    if( _gst != nullptr )
    {
        (_gst)(m_sSimTypeAll);
        if (!MatchSimType(m_sSimTypeAll)) 
        {
            LogSimTypes(m_sSimTypeAll);
            LOG_WARN_F( "EModule %S does not support current disease SimType %s \n", pFilename, m_sSimType);
            success = false;
        }
    }
    else
    {
#ifdef WIN32
        LOG_INFO_F ("GetProcAddress failed for GetSupportedSimTypes: %d.\n", GetLastError() );
#else
        LOG_INFO_F ("GetProcAddress failed for GetSupportedSimTypes: %d.\n", errno);
#endif
        success = false;
    }
    return success ;
}

bool DllLoader::GetReportInstantiator( const TCHAR* pFilename, 
                                       HMODULE repDll, 
                                       Kernel::report_instantiator_function_t* pRIF )
{
    bool success = false ;
    LOG_DEBUG_F("Calling GetProcAddress for GetReportInstantiator on %S\n", pFilename);
    typedef void (*gri)(Kernel::report_instantiator_function_t* pif);
    gri gri_func = (gri)GetProcAddress( repDll, "GetReportInstantiator" );
    if( gri_func != nullptr )
    {
        gri_func( pRIF );
        if( *pRIF != nullptr )
        {
            success = true ;
        }
        else
        {
            LOG_WARN_F( "Failed to get Report Instantiator on %S.\n", pFilename );
        }
    }
    else
    {
        LOG_WARN_F( "GetReportInstantiator not supported in %S.\n", pFilename );
    }
    return success ;
}

bool DllLoader::GetType( const TCHAR* pFilename, 
                         HMODULE repDll, 
                         std::string& rClassName )
{
    bool success = true ;

    typedef char* (*gtp)();
    gtp gtp_func = (gtp)GetProcAddress( repDll, "GetType" );
    if( gtp_func != nullptr )
    {
        char* p_class_name = gtp_func();
        if( p_class_name != nullptr )
        {
            rClassName = std::string( p_class_name );
            LOG_INFO_F( "Found Report DLL = %s\n", p_class_name );
        }
        else
        {
            LOG_WARN_F( "GetProcAddr failed for GetType on %S.\n", pFilename );
            success = false ;
        }
    }
    else
    {
        LOG_WARN_F( "GetType not supported in %S.\n", pFilename );
        success = false ;
    }
    return success ;
}


bool DllLoader::LoadInterventionDlls(const char* dllName)
{
    LOG_DEBUG("Enter LoadInterventionDlls\n");
    bool bRet = false;
    if (!EnvPtr)
    {
        LOG_ERR("LoadInterventionDlls: EnvPtr not initialized. Returning.");
        return bRet;
    }

    std::list< std::wstring > iv_dll_dirs;
    ReadEmodulesJson( INTERVENTION_EMODULES, iv_dll_dirs );

#if defined(WIN32)
    std::vector< std::wstring> allMatchingDLLs = GetAllMatchingDllsInDirs( iv_dll_dirs, interventionPlugins, dllName );
    for (std::wstring aMatchingDll : allMatchingDLLs)
    {
        LOG_INFO_F("Calling LoadLibrary for %S\n", aMatchingDll.c_str());
        HMODULE disDll = LoadLibrary( aMatchingDll.c_str() );
        
        if( CheckDynamicLibrary_Intervention( disDll, wstringToString(aMatchingDll) ) == true )
        {
            bRet = true;
        }
    }
#else
    std::vector<std::string> allMatchingSharedObjs = GetAllMatchingSharedObjectsInDirs( iv_dll_dirs, interventionPlugins, dllName );
    void* disDll = nullptr;
    for (std::string aMatchingSharedObject : allMatchingSharedObjs)
    {
        LOG_INFO_F("Loading Library: %S\n", aMatchingSharedObject.c_str());
        disDll = dlopen( aMatchingSharedObject.c_str(), RTLD_NOW );
        
        if( CheckDynamicLibrary_Intervention( disDll, aMatchingSharedObject ) == true )
        {
            bRet = true;
        }
    }
#endif

    return bRet;
}

bool DllLoader::IsValidVersion(const char* emodVer)
{
    bool bValid = false;
    if (!emodVer)
    {
        // if emodule has null version, something is wrong
        return bValid;
    }

    ProgDllVersion pv;
    if (pv.checkProgVersion(emodVer) >= 0)
    {
        bValid = true;
    }
    else
    {
        LOG_INFO_F("The application has version %s while the emodule has version %s\n", pv.getVersion(), emodVer);
    }

    return bValid;
}

bool DllLoader::MatchSimType(char* simTypes[])
{
    bool bMatched = true;
    if (!m_sSimType)
    {
        // The Dll loader doesn't care about the SimType, so return bMatched=true
        return bMatched;
    }

    int i=0;
    bMatched = false;
    while (simTypes[i] != nullptr && i < SIMTYPES_MAXNUM)
    {
        if( (simTypes[i][0] == '*') || (strcmp(simTypes[i], m_sSimType) == 0) )
        {
            bMatched = true;
            break;
        }
        i++;
    }

    return bMatched;
}

void DllLoader::LogSimTypes(char* simTypes[])
{
    int i=0;
    while (simTypes[i] != nullptr && i < SIMTYPES_MAXNUM)
    {
        LOG_INFO_F("SimType: %s \n", simTypes[i]);
        i++;
    }

}

std::string DllLoader::GetEModulePath(const char* emoduleDir)
{
    std::string sDllRootPath = ".";
    if (EnvPtr)
    {
        sDllRootPath = EnvPtr->DllPath;
    }
    return FileSystem::Concat( sDllRootPath, std::string(emoduleDir) );
}

bool DllLoader::GetEModulesVersion(const char* dllPath, list<string>& dllNames, list<string>& dllVersions)
{

    if (!dllPath || dllPath[0] == '\0')
    {
        LOG_DEBUG("The EModule root path is not given, so nothing to get version from.\n");
        return false;
    }

#ifdef WIN32

   return GetDllsVersion(dllPath, std::wstring( DISEASE_PLUGINS ), dllNames, dllVersions)
          && GetDllsVersion(dllPath, std::wstring( REPORTER_PLUGINS ), dllNames, dllVersions)
          && GetDllsVersion(dllPath, std::wstring( INTERVENTION_PLUGINS ), dllNames, dllVersions);
#else
   std::wstring diseasePluginsStr = stringToWstring( DISEASE_PLUGINS );
   std::wstring reporterPluginsStr = stringToWstring( REPORTER_PLUGINS );
   std::wstring interventionPluginsStr = stringToWstring( INTERVENTION_PLUGINS );
   return GetDllsVersion(dllPath, diseasePluginsStr, dllNames, dllVersions)
          && GetDllsVersion(dllPath, reporterPluginsStr, dllNames, dllVersions)
          && GetDllsVersion(dllPath, interventionPluginsStr, dllNames, dllVersions);
#endif

}

bool DllLoader::StringEquals(const std::wstring& wStr, const char* cStr) 
{ 
    std::string str(cStr); 

    if (wStr.size() < str.size()) 
    {
        return false; 
    }

    return std::equal(str.begin(), str.end(), wStr.begin()); 
}

bool DllLoader::StringEquals(const TCHAR* tStr, const char* cStr) 
{ 
    std::wstring wStr(tStr);
    return StringEquals(wStr, cStr);
}

std::wstring DllLoader::GetFullDllPath(std::wstring& pluginDir, const char* dllPath)
{
    LOG_DEBUG( "GetFullDllPath\n" );
    std::string sDllRootPath = "";

    if (dllPath)
    {
        LOG_DEBUG( "dllPath passed in\n" );
        sDllRootPath = dllPath;
    }
    else if (EnvPtr)
    {
        LOG_DEBUG( "dllPath not passed in, getting from EnvPtr\n" );
        release_assert( EnvPtr );
        sDllRootPath = EnvPtr->DllPath;
    }
    LOG_DEBUG( "Trying to copy from string to wstring.\n" );
    std::wstring wsDllRootPath;
    wsDllRootPath.assign(sDllRootPath.begin(), sDllRootPath.end());
    LOG_DEBUG_F("DLL ws root path: %S\n", wsDllRootPath.c_str());
    return FileSystem::Concat( wsDllRootPath, pluginDir );
}

bool DllLoader::CheckEModuleVersion(HMODULE hEMod, char* emodVer)
{
    bool bRet = false;
    char* erro;
    if (!EnvPtr)
    {
        LOG_ERR("CheckEModuleVersion: EnvPtr not initialized. Returning.\n");
        return false;
    }

    gver _gver = (gver)GetProcAddress( hEMod, "GetEModuleVersion" );
    char* error = dlerror();
    if ( error != NULL)
    {
        LOG_WARN_F("CheckEModuleVersion: Got error while trying to GetProcAddress of function GetEModuleVersion(): %S.\n", error);
    }
    if( _gver != nullptr)
    {
        char emodVersion[64];
        char* emodVersion1 = new char[64];
        for(int i=0; i<63; i++) emodVersion1[i] = ' ';
        emodVersion1[63] = '\0';
        try{
        (*_gver)(emodVersion1,EnvPtr);
        } catch (const char* strException) {
            cerr << "Error: " << strException << endl;
        } catch (...) {
            cout << "Unknown exception" << endl;
        }
        bRet = IsValidVersion(emodVersion1);
        if (emodVer)
        {
            strcpy(emodVer, emodVersion1);
        }
    }
    else
    {
        LOG_WARN("GetProcAddr failed for GetEModuleVersion.\n");
    }
    LOG_INFO_F("CheckEModuleVersion: Success in calling GetEModuleVersion(), got version as: %S.\n", emodVer);
    return bRet;
}

bool DllLoader::GetDllsVersion(const char* dllPath, std::wstring& wsPluginDir,list<string>& dllNames, list<string>& dllVersions)
{    
    // Look through disease dll directory, do LoadLibrary on each .dll, do GetProcAddress for get
    std::wstring dllDir = GetFullDllPath(wsPluginDir, dllPath);

#ifdef WIN32
    std::wstring dllDirStar = FileSystem::Concat( dllDir, std::wstring(L"*") );
    std::list<std::wstring> dll_dirs;
    dll_dirs.push_back(dllDirStar);
    std::vector<std::wstring> matchingDlls = GetAllMatchingDllsInDirs(dll_dirs);
    for(std::wstring dllPath : matchingDlls) {
        LOG_DEBUG_F("Calling LoadLibrary for %S\n", dllPath.c_str());
        HMODULE ecDll = LoadLibrary( dllPath.c_str() );
        if( ecDll == nullptr )
        {
            LOG_WARN_F("Failed to load dll %S\n",dllPath);
        }
        else
        {

            LOG_DEBUG("Calling GetProcAddress for GetEModuleVersion\n");
            char emodVersion[64];
            if (!CheckEModuleVersion(ecDll, emodVersion))
            {
                LOG_WARN_F("The version of EModule %S is lower than current application\n", dllPath);
            }
            dllVersions.push_back(emodVersion);
            
            std::string dllName = WstringToString(dllPath);
            dllNames.push_back(dllName);
        }
    }
#else
    std::wstring &dllDirStar = dllDir;
    std::string starStr("/*");
    dllDirStar.append( stringToWstring(starStr) );
    std::list<std::wstring> dll_dirs;
    dll_dirs.push_back( dllDirStar );
    std::vector<std::string> matchingSharedObjects = GetAllMatchingSharedObjectsInDirs(dll_dirs);
    for(std::string dllPath : matchingSharedObjects) {
        LOG_INFO_F("Loading Library: %S\n", dllPath.c_str());
        void* disDll = dlopen( dllPath.c_str(), RTLD_NOW );
        if( disDll == nullptr )
        {
            LOG_WARN_F("Failed to load library %S\n", dllPath);
        }
        else
        {
            LOG_INFO("Calling GetProcAddr for GetEModuleVersion\n");
            char emodVersion[64];
            if (!CheckEModuleVersion(disDll, emodVersion))
            {
                LOG_WARN_F("The version of EModule %S is lower than current application!\n", dllPath);
            }
            dllVersions.push_back(emodVersion);
            dllNames.push_back(dllPath);
        }
    }
#endif
    
    return true;
}

json::Object
DllLoader::GetDiseaseDllSchemas()
{
    LOG_INFO_F( "GetDiseaseDllSchemas: # of GetSchema func pointers = %d\n", getSchemaFuncPtrMap.size() );

    json::Object configSchemaAllJson;
    std::ostringstream configSchemaAllString;
    for (auto& entry : getSchemaFuncPtrMap)
    {
        const std::string& sim_type = entry.first;
        LOG_DEBUG_F( "sim_type = %s\n", sim_type.c_str() );
        getSchema fpGetSchema = entry.second;

        // We get the schema serialized (as a string) and then deserialize. A bit of arguably unnecessary
        // overhead, but I'd rather transfer strings over the dll 'boundary' than json objects.
        (*fpGetSchema)();
        const char * config_schema = getenv( "GET_SCHEMA_RESULT" );
        std::stringstream sim_schema( config_schema, std::ios::in);
        json::Object simSchemaJson;
        json::Reader::Read( simSchemaJson, sim_schema );
        LOG_DEBUG_F( "config_schema = %s\n", config_schema );
        simSchemaJson[ "version" ] = json::String(dll2VersionStringMap[ sim_type ]);
        configSchemaAllJson[ std::string( sim_type ) + ":emodule" ] = simSchemaJson;
    }
    json::Writer::Write( configSchemaAllJson, configSchemaAllString );
    return configSchemaAllJson;
}

#ifdef WIN32
std::vector<std::wstring> DllLoader::GetAllMatchingDllsInDirs(std::list< std::wstring > dirsToSearch, const char* dllName)
{
    std::vector<std::wstring> allMatchingDLLs;
    for (auto& dllDirStar : dirsToSearch)
    {
        LOG_INFO_F("Searching disease plugin dir: %S\n", dllDirStar.c_str());

        WIN32_FIND_DATA ffd;
        HANDLE hFind = FindFirstFile(dllDirStar.c_str(), &ffd);
        do
        {
            LOG_INFO_F("fa=%d; fn=%S, fattr=%d, fadir=%d\n", ffd.dwFileAttributes, ffd.cFileName, ffd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
            if ( ffd.dwFileAttributes > 0 && ffd.cFileName[0] != '\0' && !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) // ignore . and ..
            {
                std::wstring dllPath = dllDirStar;
                if( fully_qualified_dll_path == false )
                {
                    LOG_INFO( "We are using reg-ex dll searching, append dir path to filename found.\n" );
                    dllPath = FileSystem::Concat( dllDir, std::wstring(ffd.cFileName) );
                }
                LOG_DEBUG_F("Found dll: %S\n", dllPath.c_str());

                // must end in .dll
                if( dllPath.find( L".dll" ) == std::string::npos ||
                    dllPath.find( L".dll" ) != dllPath.length()-4  ||
                    (dllName && !StringEquals(ffd.cFileName, dllName)) )
                {
                    if (dllName)
                        LOG_INFO_F( "%S is not a DLL or doesn't match %s\n", dllPath, dllName );
                    else
                        LOG_INFO_F( "%S is not a DLL\n", dllPath );
                    continue;
                }
                
                allMatchingDLLs.push_back(dllPath);

            }
            else
            {
                LOG_INFO("That was a directory not an EModule or . or .. so ignore.\n");
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);
    }

    return allMatchingDLLs;
}

std::vector<std::wstring> DllLoader::GetAllMatchingDllsInDirs(std::list< std::wstring > dll_dirs, std::wstring driPlugins, const char* dllName)
{
    std::wstring dllDir( L" " );
    if( dll_dirs.size() == 0 )
    {
        LOG_INFO( "Getting PLUGINS dir because no paths from emodules_map.json\n" );
        dllDir = GetFullDllPath(driplugins);
        std::wstring &dllDirStar = FileSystem::Concat( dllDir, std::wstring(L"*") );
        dll_dirs.push_back( dllDirStar );
    }
    else
    {
        LOG_INFO( "Did not use --dll-path since emodules_map.json file was found.\n" );
    }
    
    return GetAllMatchingDllsInDirs(dll_dirs, dllName);
}

#else
std::vector<std::string> DllLoader::GetAllMatchingSharedObjectsInDirs(std::list< std::wstring > dirsToSearch, const char* dllName)
{
    std::vector<std::string> allMatchingSoFiles;
    for (auto& dllDirStar : dirsToSearch) {
        LOG_INFO_F("Searching disease plugin dir: %S\n", dllDirStar.c_str());
        DIR *dp;
        struct dirent *dirp;
        std::string dllDirStarStr = WstringToString(dllDirStar);
        
        struct stat s;
        if ( stat(dllDirStarStr.c_str(), &s) == 0 )
        {
            if (S_ISREG(s.st_mode))
            {
                if( dllDirStarStr.find( ".so" ) == std::string::npos ||
                     dllDirStarStr.find( ".so" ) != dllDirStarStr.length()-3  ||
                     ( dllName && dllDirStarStr.compare(dllName) != 0 ) )
                {
                    if (dllName)
                        LOG_INFO_F( "%S is not a .SO or doesn't match %s\n", dllDirStarStr, dllName );
                    else
                        LOG_INFO_F( "%S is not a .SO\n", dllDirStarStr );
                    continue;
                }

                allMatchingSoFiles.push_back(dllDirStarStr);
                LOG_INFO_F("Found shared object file: %S\n", dllDirStar.c_str());
            }
            else
            {
                 LOG_INFO("That was a file .. so ignore.\n");
            }
        }
    }

    return allMatchingSoFiles;
}

std::vector<std::string> DllLoader::GetAllMatchingSharedObjectsInDirs(std::list< std::wstring > dll_dirs, std::wstring driplugins, const char* dllName)
{
    if( dll_dirs.size() == 0 )
    {
        LOG_INFO( "Getting PLUGINS dir because no paths from emodules_map.json\n" );
        std::wstring dllDir = GetFullDllPath(driplugins);
        std::wstring dllDirStar = dllDir;
        std::string starStr("/*");
        dllDirStar.append( stringToWstring(starStr) );
        dll_dirs.push_back( dllDirStar );
    }
    else
    {
        LOG_INFO( "Did not use --dll-path since emodules_map.json file was found.\n" );
    }

    return GetAllMatchingSharedObjectsInDirs(dll_dirs, dllName);
}
#endif

std::wstring DllLoader::stringToWstring(std::string t_str)
{
    typedef std::codecvt_utf8<wchar_t> convert_type;
    std::wstring_convert<convert_type, wchar_t> converter;
    std::wstring converted_str = converter.from_bytes(t_str);
    return converted_str;
}

std::string DllLoader::WstringToString(std::wstring w_str)
{
    typedef std::codecvt_utf8<wchar_t> convert_type;
    std::wstring_convert<convert_type, wchar_t> converter;
    std::string converted_str = converter.to_bytes( w_str );
    return converted_str;
}

bool DllLoader::checkFileExists(const std::string& name)
{
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

bool DllLoader::CheckDynamicLibrary_Disease(void* disDll, std::string aMatchingLibrary, std::map< std::string, createSim>& createSimFuncPtrMap)
{
    if( disDll == nullptr )
    {
        LOG_WARN_F("Failed to load library %S\n", aMatchingLibrary);
        return false;
    }
    else
    {
        LOG_INFO("Calling GetProcAddr for GetEModuleVersion\n");
        char emodVersion[64];
        if (!CheckEModuleVersion(disDll, emodVersion))
        {
            LOG_WARN_F("The version of EModule %S is lower than current application.\n", aMatchingLibrary); // this message should be differentiable for GetProcAddr fail
            //return false;
        }

        LOG_INFO("Calling GetProcAddress for GetDiseaseType\n");
        typedef const char * (*gdt)();
        gdt _gdt = (gdt)GetProcAddress( disDll, "GetDiseaseType" );
        std::string diseaseType = std::string("");
        if( _gdt != nullptr )
        {
            diseaseType = (_gdt)();
        }
        else
        {
            LOG_WARN("GetProcAddr failed for GetDiseaseType.\n");
            return false;
        }

        dll2VersionStringMap[ diseaseType ] = std::string( emodVersion );

        LOG_INFO("Calling GetProcAddress for CreateSimulation\n");
        createSim _createSim = (createSim)GetProcAddress( disDll, "CreateSimulation" );
        if( _createSim != nullptr )
        {
            LOG_INFO_F("Caching create_sim function pointer for disease type: %s\n", diseaseType.c_str());
            createSimFuncPtrMap[diseaseType] = _createSim;
        }
        else
        {
            LOG_WARN("GetProcAddr failed for CreateSimulation.\n");
            return false;
        }

        LOG_INFO("Calling GetProcAddress for GetSchema\n");
        getSchema _getSchema = (getSchema)GetProcAddress( disDll, "GetSchema" );
        if( _getSchema != nullptr )
        {
            LOG_INFO_F("Caching get_schema function pointer for disease type: %s\n", diseaseType.c_str());
            getSchemaFuncPtrMap[diseaseType] = _getSchema;
        }
        else
        {
            LOG_WARN("GetProcAddr failed for GetSchema.\n");
            return false;
        }
    }
    
    return true;
}

bool DllLoader::CheckDynamicLibrary_Reporter(void* disDll, std::string aMatchingLibrary, std::unordered_map< std::string, Kernel::report_instantiator_function_t >& reportInstantiators)
{
    if( disDll == nullptr )
    {
        LOG_WARN_F("Failed to load library %S\n", aMatchingLibrary);
        return false;
    }
    else
    {

        LOG_INFO("Calling GetProcAddr for GetEModuleVersion\n");
        char emodVersion[64];
        if (!CheckEModuleVersion(disDll, emodVersion))
        {
            LOG_WARN_F("The version of EModule %S is lower than current application!\n", aMatchingLibrary);
            // For now, load report dll anyway
            //continue;
        }
        
        const TCHAR* tc_MatchingLibrary = stringToWstring(aMatchingLibrary).c_str();

        LOG_INFO("Calling GetSimTypes\n");
        if ( GetSimTypes( tc_MatchingLibrary, disDll ) == false)
        {
            return false;
        }

        Kernel::report_instantiator_function_t rif = nullptr ;
        if ( GetReportInstantiator( tc_MatchingLibrary, disDll, &rif ) == false ) {
            return false;
        }
        if( rif != nullptr )
        {
            std::string class_name ;
            if ( GetType( tc_MatchingLibrary, disDll, class_name ) == false )
            {
            return false;
            }

            reportInstantiators[ class_name ] = rif ;
        }
    }
    return true;
}

bool DllLoader::CheckDynamicLibrary_Intervention(void* disDll, std::string aMatchingLibrary)
{
    if( disDll == nullptr )
    {
        LOG_WARN_F("Failed to load library %S\n", aMatchingLibrary);
        return false;
    }
    else
    {

        LOG_INFO("Calling GetProcAddr for GetEModuleVersion\n");
        char emodVersion[64];
        if (!CheckEModuleVersion(disDll, emodVersion))
        {
            LOG_WARN_F("The version of EModule %S is lower than current application!\n", aMatchingLibrary);
            // For now, load report dll anyway
            //return false;
        }
        
        LOG_INFO("Calling GetProcAddress for RegisterWithFactory...\n");
        typedef int (*callProc)(Kernel::IInterventionFactory *);
        callProc _callProc = (callProc)GetProcAddress( disDll, "RegisterWithFactory" );
        if( _callProc != nullptr )
        {
    	    (_callProc)( Kernel::InterventionFactory::getInstance() );
        }
        else
        {
            LOG_WARN_F("GetProcAddr failed for RegisterWithFactory for filename %s.\n", aMatchingLibrary);
            return false;
        }
    }
    return true;
}
