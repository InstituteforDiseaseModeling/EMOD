/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
#endif

typedef char* (*gver)(char*,const Environment * pEnv);

static const char* _module = "DllLoader";
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
    std::ifstream emodules_json_raw;

    LOG_DEBUG("open emodules_map.json\n");
    emodules_json_raw.open( "emodules_map.json" );
    if (emodules_json_raw.fail())
    {
        // actually, might be ok if just using --dll_dir, not settled yet
        LOG_INFO( "ReadEmodulesJson: no file, returning.\n" );
        return;
    }

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
    std::wstring dllDir( L" " );
    bool fully_qualified_dll_path = true;
    if( disease_dll_dirs.size() == 0 )
    {
        LOG_INFO( "Getting DISEASE_PLUGINS dir because no paths from emodules_map.json\n" );
        dllDir = GetFullDllPath(std::wstring(DISEASE_PLUGINS));
        std::wstring &dllDirStar = FileSystem::Concat( dllDir, std::wstring(L"*") );
        disease_dll_dirs.push_back( dllDirStar );
        fully_qualified_dll_path = false;
    }
    else
    {
        LOG_INFO( "Did not use --dll-path since emodules_map.json file was found.\n" );
    }

    for (auto& dllDirStar : disease_dll_dirs)
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
                LOG_INFO_F("Calling LoadLibrary for %S\n", dllPath.c_str());
                HMODULE disDll = LoadLibrary( dllPath.c_str() );
                if( disDll == nullptr )
                {
                    LOG_WARN_F("Failed to load dll %S\n", ffd.cFileName);
                }
                else
                {
                    LOG_INFO("Calling GetProcAddr for GetEModuleVersion\n");
                    char emodVersion[64];
                    if (!CheckEModuleVersion(disDll, emodVersion))
                    {
                        LOG_WARN_F("The version of EModule %S is lower than current application.\n", ffd.cFileName); // this message should be differentiable for GetProcAddr fail
                        //continue;
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
                        continue;
                    }

                    dll2VersionStringMap[ diseaseType ] = std::string( emodVersion );

                    LOG_INFO("Calling GetProcAddress for CreateSimulation\n");
                    createSim _createSim = (createSim)GetProcAddress( disDll, "CreateSimulation" );
                    if( _createSim != nullptr )
                    {
                        LOG_INFO_F("Caching create_sim function pointer for disease type: %s\n", diseaseType.c_str());
                        createSimFuncPtrMap[diseaseType] = _createSim;
                        bRet = true;
                    }
                    else
                    {
                        LOG_WARN("GetProcAddr failed for CreateSimulation.\n");
                        continue;
                    }

                    LOG_INFO("Calling GetProcAddress for GetSchema\n");
                    getSchema _getSchema = (getSchema)GetProcAddress( disDll, "GetSchema" );
                    if( _getSchema != nullptr )
                    {
                        LOG_INFO_F("Caching get_schema function pointer for disease type: %s\n", diseaseType.c_str());
                        getSchemaFuncPtrMap[diseaseType] = _getSchema;
                        bRet = true;
                    }
                    else
                    {
                        LOG_WARN("GetProcAddr failed for GetSchema.\n");
                        continue;
                    }
                }
            }
            else
            {
                LOG_INFO("That was a directory not an EModule or . or .. so ignore.\n");
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);
    }

#else
#warning "Linux shared library for disease not implemented yet at load."
#endif

    return bRet;
}

bool DllLoader::LoadReportDlls( std::unordered_map< std::string, Kernel::report_instantiator_function_t >& reportInstantiators,
                                const char* dllName)
{
    bool bRet = false;
    if (!EnvPtr) return bRet;

    std::list< std::wstring > report_dll_dirs;
    ReadEmodulesJson( REPORTER_EMODULES, report_dll_dirs );

#ifdef WIN32

    std::wstring dllDir( L" " );
    bool fully_qualified_dll_path = true;
    if( report_dll_dirs.size() == 0 )
    {
        LOG_DEBUG( "Getting REPORTER_PLUGINS dir because no paths from emodules_map.json\n" );
        dllDir = GetFullDllPath(std::wstring(REPORTER_PLUGINS)); 
        std::wstring &dllDirStar = FileSystem::Concat( dllDir, std::wstring(L"*") );
        report_dll_dirs.push_back( dllDirStar );
        fully_qualified_dll_path = false;
    }
    else
    {
        LOG_DEBUG( "Did not use --dll-path since emodules_map.json file was found.\n" );
    }

    WIN32_FIND_DATA ffd;

    for (auto& dllDirStar : report_dll_dirs)
    {
        LOG_DEBUG_F("Searching reporter plugin dir: %S\n", dllDirStar.c_str());

        HANDLE hFind = FindFirstFile(dllDirStar.c_str(), &ffd);
        do
        {
            if ( ffd.dwFileAttributes > 0 && ffd.cFileName[0] != '\0' && !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) // ignore . and ..
            {
                std::wstring dllPath = dllDirStar;
                LOG_DEBUG_F("Found dll: %S\n", dllPath.c_str());
                if( fully_qualified_dll_path == false )
                {
                    LOG_DEBUG( "We are using reg-ex dll searching, append dir path to filename found.\n" );
                    dllPath = FileSystem::Concat( dllDir, std::wstring(ffd.cFileName) );
                }

                if( dllPath.find( L".dll" ) == std::string::npos || // must end in .dll
                    dllPath.find( L".dll" ) != dllPath.length()-4 ||
                    (dllName && !StringEquals(ffd.cFileName, dllName)) )
                {
                    if (dllName)
                        LOG_DEBUG_F( "%S is not a DLL or doesn't match %s\n", dllPath, dllName );
                    else
                        LOG_DEBUG_F( "%S is not a DLL\n", dllPath );
                    continue;
                }

                LOG_INFO_F( "Calling LoadLibrary on reporter emodule %S\n", dllPath.c_str() );
                HMODULE repDll = LoadLibrary( dllPath.c_str() );

                if( repDll == nullptr )
                {
                    LOG_WARN_F( "Failed to load dll %S\n", ffd.cFileName );
                }
                else
                {

                    LOG_INFO("Calling GetProcAddr for GetEModuleVersion\n");
                    char emodVersion[64];
                    if (!CheckEModuleVersion(repDll, emodVersion))
                    {
                        LOG_WARN_F("The version of EModule %S is lower than current application!\n", ffd.cFileName);

                        // For now, load report dll anyway
                        //continue;
                    }

                    bool success = GetSimTypes( ffd.cFileName, repDll );
                    if( !success ) continue ;

                    Kernel::report_instantiator_function_t rif = nullptr ;
                    success = GetReportInstantiator( ffd.cFileName, repDll, &rif );
                    if( !success ) continue ;

                    if( rif != nullptr )
                    {
                        std::string class_name ;
                        success = GetType( ffd.cFileName, repDll, class_name );
                        if( !success ) continue ;

                        reportInstantiators[ class_name ] = rif ;
                        bRet = true ;
                    }
                }
            }
            else
            {
                LOG_DEBUG("That was a directory not an EModule or . or .. so ignore.\n");
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);
    }
#else
#warning "Report plugin loading (dlopen/dlsym) not implemented on linux yet."
#endif

    return bRet;
}


#if defined(WIN32)
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
        LOG_INFO_F ("GetProcAddress failed for GetSupportedSimTypes: %d.\n", GetLastError() );
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
#endif

bool DllLoader::LoadInterventionDlls(const char* dllName)
{
    bool bRet = false;
    if (!EnvPtr) return bRet;

    std::list< std::wstring > iv_dll_dirs;
    ReadEmodulesJson( INTERVENTION_EMODULES, iv_dll_dirs );

#ifdef WIN32
    std::wstring dllDir( L" " );
    bool fully_qualified_dll_path = true;
    if( iv_dll_dirs.size() == 0 )
    {
        LOG_DEBUG( "Getting INTERVENTION_PLUGINS dir because no paths from emodules_map.json\n" );
        dllDir = GetFullDllPath(std::wstring(INTERVENTION_PLUGINS)); 
        LOG_DEBUG_F("Found intervention plugin dir: %S\n", dllDir.c_str()); // ???
        std::wstring &dllDirStar = FileSystem::Concat( dllDir, std::wstring(L"*") );
        iv_dll_dirs.push_back( dllDirStar );
        fully_qualified_dll_path = false;
    }
    else
    {
        LOG_DEBUG( "Did not use --dll-path since emodules_map.json file was found.\n" );
    }


    for (auto& dllDirStar : iv_dll_dirs)
    {
        LOG_DEBUG_F("Searching intervention plugin dir: %S\n", dllDirStar.c_str());

        WIN32_FIND_DATA ffd;
        HANDLE hFind = FindFirstFile(dllDirStar.c_str(), &ffd);
        do
        {
            if (ffd.dwFileAttributes > 0 && ffd.cFileName[0] != '\0' && !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) // ignore . and ..
            {
                std::wstring dllPath = dllDirStar;
                if( fully_qualified_dll_path == false )
                {
                    LOG_DEBUG( "We are using reg-ex dll searching, append dir path to filename found.\n" );
                    dllPath = FileSystem::Concat( dllDir,  std::wstring(ffd.cFileName) );
                }
                LOG_DEBUG_F("Found dll: %S\n", dllPath.c_str());

                // must end in .dll
                if( dllPath.find( L".dll" ) == std::string::npos ||
                    dllPath.find( L".dll" ) != dllPath.length()-4 ||
                    (dllName && !StringEquals(ffd.cFileName, dllName)) )
                {
                    if (dllName)
                        LOG_DEBUG_F( "%S is not a DLL or doesn't match %s\n", dllPath, dllName );
                    else
                        LOG_DEBUG_F( "%S is not a DLL\n", dllPath );
                    continue;
                }

                LOG_INFO_F("Calling LoadLibrary on interventions emodule %S\n", dllPath.c_str());
                HMODULE intvenDll = LoadLibrary( dllPath.c_str() );
                if( intvenDll == nullptr )
                {
                    LOG_WARN_F("Failed to load dll %S\n", ffd.cFileName);
                }
                else
                {
                    LOG_INFO("Calling GetProcAddr for GetEModuleVersion\n");
                    char emodVersion[64];
                    if (!CheckEModuleVersion(intvenDll, emodVersion))
                    {
                        LOG_WARN_F("The version of EModule %S is lower than current application!\n", ffd.cFileName);

                        // For now, load report dll anyway
                        //continue;
                    }

                    LOG_INFO("Calling GetProcAddress for RegisterWithFactory...\n");
                    typedef int (*callProc)(Kernel::IInterventionFactory *);
                    callProc _callProc = (callProc)GetProcAddress( intvenDll, "RegisterWithFactory" );
                    if( _callProc != nullptr )
                    {
                        (_callProc)( Kernel::InterventionFactory::getInstance() );
                        bRet = true;
                    }
                    else
                    {
                        LOG_WARN_F("GetProcAddr failed for RegisterWithFactory for filename %s.\n", std::wstring(ffd.cFileName).c_str());
                    }
                }
            }
            else
            {
                LOG_DEBUG("That was a directory not an EModule or . or .. so ignore.\n");
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);
    }

#else // WIN32

    // Scan for intervention dlls/shared libraries and Register them with us.
    void * newSimDlHandle = nullptr;
    int (*RegisterDotsoIntervention)( Kernel::InterventionFactory* );
    DIR * dp;
    struct dirent *dirp;
    if( ( dp = opendir( "/var/opt/plugins/interventions" ) ) == nullptr )
    {
        LOG_WARN("Failed to open interventions plugin directory.\n");
        return;
    }
    while( ( dirp = readdir( dp ) ) != nullptr )
    {
        if( std::string( dirp->d_name ) == "." ||
            std::string( dirp->d_name ) == ".." )
        {
            continue;
        }
        LOG_DEBUG_F("Considering %s\n", dirp->d_name);
        std::string fullDllPath = FileSystem::Concat( std::string("/var/opt/plugins/interventions/"), std::string( dirp->d_name ) );
        newSimDlHandle = dlopen( fullDllPath.c_str(), RTLD_LAZY );
        if( newSimDlHandle )
        {
            RegisterDotsoIntervention = dlsym( newSimDlHandle, "RegisterWithFactory" );
            if( RegisterDotsoIntervention )
            {
                //RegisterDotsoIntervention( this );
            }
            else
            {
                LOG_DEBUG("dlsym failed\n");
            }
        }
        else
        {
            LOG_WARN_F("dlopen failed on %s with error %s\n", fullDllPath.c_str(), dlerror());
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
#warning "Linux shared library for disease not implemented yet at load."
#endif
}

#if defined(WIN32)
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
        LOG_INFO( "dllPath passed in\n" );
        sDllRootPath = dllPath;
    }
    else if (EnvPtr)
    {
        LOG_INFO( "dllPath not passed in, getting from EnvPtr\n" );
        release_assert( EnvPtr );
        sDllRootPath = EnvPtr->DllPath;
    }
    LOG_INFO( "Trying to copy from string to wstring.\n" );
    std::wstring wsDllRootPath;
    wsDllRootPath.assign(sDllRootPath.begin(), sDllRootPath.end());
    LOG_INFO_F("DLL ws root path: %S\n", wsDllRootPath.c_str());
    return FileSystem::Concat( wsDllRootPath, pluginDir );
}

bool DllLoader::CheckEModuleVersion(HMODULE hEMod, char* emodVer)
{
    bool bRet = false;
    gver _gver = (gver)GetProcAddress( hEMod, "GetEModuleVersion" );
    if( _gver != nullptr )
    {
        char emodVersion[64];
        (_gver)(emodVersion,EnvPtr);
        bRet = IsValidVersion(emodVersion);
        if (emodVer)
        {
            strcpy(emodVer, emodVersion);
        }
    }
    else
    {
        LOG_WARN("GetProcAddr failed for GetEModuleVersion.\n");
    }
    return bRet;
}

bool DllLoader::GetDllsVersion(const char* dllPath, std::wstring& wsPluginDir,list<string>& dllNames, list<string>& dllVersions)
{    

    // Look through disease dll directory, do LoadLibrary on each .dll,
    // do GetProcAddress for get
    bool bRet = true;

    std::wstring dllDir = GetFullDllPath(wsPluginDir, dllPath); 
    std::wstring dllDirStar = FileSystem::Concat( dllDir, std::wstring(L"*") );
    
    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(dllDirStar.c_str(), &ffd);
    do
    {
        if ( ffd.dwFileAttributes > 0 && ffd.cFileName[0] != '\0' && !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) // ignore . and ..
        {
            std::wstring dllPath = FileSystem::Concat( dllDir, std::wstring(ffd.cFileName) );
                
            // must end in .dll
            if( dllPath.find( L".dll" ) == std::string::npos ||
                dllPath.find( L".dll" ) != dllPath.length()-4 )
            {
                LOG_INFO_F("Not a dll ( %S) \n", ffd.cFileName);
                continue;
            }
                

            LOG_INFO_F("Calling LoadLibrary for %S\n", dllPath.c_str());
            HMODULE ecDll = LoadLibrary( dllPath.c_str() );
            if( ecDll == nullptr )
            {
                LOG_WARN_F("Failed to load dll %S\n",ffd.cFileName);
            }
            else
            {

                LOG_INFO("Calling GetProcAddress for GetEModuleVersion\n");
                char emodVersion[64];
                if (!CheckEModuleVersion(ecDll, emodVersion))
                {
                    LOG_WARN_F("The version of EModule %S is lower than current application\n", ffd.cFileName);
                }
                dllVersions.push_back(emodVersion);
                                   
                // Convert to a char*
                size_t wcsize = wcslen(ffd.cFileName) + 1;
                const size_t newsize = 256;
                size_t convertedChars = 0;
                char namestring[newsize];
                wcstombs_s(&convertedChars, namestring, wcsize, ffd.cFileName, _TRUNCATE);
                dllNames.push_back(namestring);

            }
        }
        else
        {
            LOG_DEBUG("That was a directory not an EModule or . or .. so ignore.\n");
        }
    }
    while (FindNextFile(hFind, &ffd) != 0);

    return bRet;

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
#else
json::Object
DllLoader::GetDiseaseDllSchemas()
{
}
#endif // End of WIN32
