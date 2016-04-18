/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include <stdafx.h>
#include "stdio.h" // remove()
#include <sys/stat.h> // stat()
#include <algorithm> //std::replace()
#include <fstream>

#include "Exceptions.h"
#include "FileSystem.h"

#ifdef _WIN32
#include <direct.h> // _mkdir(), _rmdir()
#else
#include <unistd.h> // getcwd() definition
#include <sys/param.h> // MAXPATHLEN definition
#endif

static char FILE_SEPARATOR = '/' ; // this works on both windows and linux
static std::string TRAIL_CHARS(" \n\r\t\\/");


// Instantiate the the std:string and std:wstring versions of Concat()
template std::string  IDMAPI FileSystem::Concat<std::string >( const std::string&  rDirectory, const std::string&  rFileName );
template std::wstring IDMAPI FileSystem::Concat<std::wstring>( const std::wstring& rDirectory, const std::wstring& rFileName );


bool FileSystem::RemoveFile( const std::string& rFilename )
{
    int err = remove( rFilename.c_str() );
    return err == 0 ;
}

bool FileSystem::RemoveDirectory( const std::string& rDir )
{
    bool success = true ;
#ifdef WIN32
    success = _rmdir( rDir.c_str() ) == 0 ;
#else
    success = rmdir( rDir.c_str() ) == 0 ;
#endif
    return success ;
}

bool FileSystem::FileExists( const std::string& rPath )
{
    struct stat s;
    bool exists = stat( rPath.c_str(), &s ) == 0 ;
    exists = exists && (s.st_mode & S_IFREG) ; /*needed for linux, works on windows*/
    return exists ;
}

bool FileSystem::DirectoryExists( const std::string& rPath )
{
    struct stat s;
    bool exists = stat( rPath.c_str(), &s ) == 0 ;
    exists = exists && (s.st_mode & S_IFDIR) ;
    return exists ;
}

std::string FileSystem::RemoveTrailingChars( const std::string& rStr )
{
    std::string ret_str = rStr ;
    // Erase all trailing whitespace and pathing characters. Note that 
    // find_last_not_of returns size_t, not an iterator, and so erase deletes to end.
    ret_str.erase(ret_str.find_last_not_of(TRAIL_CHARS)+1);
    return ret_str ;
}

std::wstring FileSystem::RemoveTrailingChars( const std::wstring& rStr )
{
    std::wstring w_trail_chars ;
    w_trail_chars.assign( TRAIL_CHARS.begin(), TRAIL_CHARS.end() );

    std::wstring ret_str = rStr ;
    // Erase all trailing whitespace and pathing characters. Note that 
    // find_last_not_of returns size_t, not an iterator, and so erase deletes to end.
    ret_str.erase(ret_str.find_last_not_of(w_trail_chars)+1);
    return ret_str ;
}

template <class T>
T FileSystem::Concat( const T& rDirectory, const T& rFilename )
{
    T dir = rDirectory;
    T fn  = rFilename ;
    std::replace( dir.begin(), dir.end(), '\\', FILE_SEPARATOR );
    std::replace( fn.begin(),  fn.end(),  '\\', FILE_SEPARATOR );

    dir = RemoveTrailingChars( dir );
    fn  = RemoveTrailingChars( fn  );

    if( (dir.size() > 0) && (dir[ dir.size() - 1 ] != FILE_SEPARATOR) )
    {
        dir += FILE_SEPARATOR ;
    }
    if( (fn.size() > 0) && (fn[0] == FILE_SEPARATOR) )
    {
        fn = fn.substr( 1, fn.size() -1 );
    }

    T path = dir + fn ;
    return path ;
}

std::string FileSystem::GetCurrentWorkingDirectory()
{
#ifdef _WIN32
    char c_path[ _MAX_PATH ] ;
    _getcwd( c_path, _MAX_PATH );

#else
    char c_path[ MAXPATHLEN ] ;
    getcwd( c_path, MAXPATHLEN );
#endif

    std::string path( c_path );
    return path ;
}

bool FileSystem::MakeDirectory( const std::string& rDirName )
{
    bool success = true ;

#ifdef _WIN32
    success =_mkdir( rDirName.c_str() ) == 0 ;
#else
    success = mkdir( rDirName.c_str(), S_IRWXU | S_IRWXG | S_IRWXO  ) == 0 ;
#endif

    return success ;
}

std::string* FileSystem::ReadFile( const char* pFilename )
{
    if( !FileSystem::FileExists( pFilename ) )
    {
        throw Kernel::FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, pFilename );
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! This needs to be ifstream so that files can be read-only.
    // !!! If you use fstream and the file is read-only, it will fail.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    std::ifstream ss ;
    ss.open( pFilename );
    if( ss.fail() )
    {
        throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, pFilename );
    }

    ss.seekg(0, std::ios::end);
    int64_t length = ss.tellg();
    ss.seekg(0, std::ios::beg);

    std::string* p_buffer = new std::string(length, '\0');

    ss.read(&((*p_buffer)[0]), length );

    ss.close();

    return p_buffer ;
}