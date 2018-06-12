/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include "IdmApi.h"

// FileSystem was created to encapsulate the cross platform issues of a set of
// file/directory functions.  We also wanted to remove the dependency on boost.
namespace FileSystem
{
    // Returns true if the given file was successfully removed.  If the file does not exist,
    // the method will return false.  This will fail on a directory.
    bool IDMAPI RemoveFile( const std::string& rFilename );

    // Returns true if the given directory was successfuly removed.  If the directory does not exist
    // or is not empty, the method will return false.  This will fail on a file.
    bool IDMAPI RemoveDirectory( const std::string& rDir );

    // Return true if the given filename exists and is for a file.  If the filename was for
    // an existing directory, the method would return false.
    bool IDMAPI FileExists( const std::string& rFilename );

    // Return true if the given filename exists in the given path.
    bool IDMAPI FileExistsInPath( const std::string& rPath, const std::string& rFilename );

    // Return true if the given directory name exists and is for a directory.  If the
    // name exists and is for a file, the method will return false.
    bool IDMAPI DirectoryExists( const std::string& rDir );

    // Return the path to the current working directory
    std::string IDMAPI GetCurrentWorkingDirectory() ;

    // Return true if the given directory was able to be created.  If something exists with this
    // directory name, the method will return false.
    bool IDMAPI MakeDirectory( const std::string& rDirName );

    // Return the concatenated directory and filename.  This method attempts to make sure the path
    // uses the correct file separators for the given OS.  It will make sure that there is only one
    // separator between the given directory and filename.
    template <class T>
    T Concat( const T& rDirectory, const T& rFilename );

    // Erase all trailing whitespace and pathing characters.
    std::string  IDMAPI RemoveTrailingChars( const std::string&  rStr );
    std::wstring IDMAPI RemoveTrailingChars( const std::wstring& rStr );

    // Read the given file and return its contents in a string
    std::string IDMAPI *ReadFile( const char* pFilename );

    // Returns a string with the error message related to the system "errno"
    std::string GetSystemErrorMessage();

    // open the file with the given name and return an input file stream
    void OpenFileForReading( std::ifstream& rInputStream, const char* pFilename, bool isBinary = false );

    // open the file with the given name and return an output file stream
    void OpenFileForWriting( std::ofstream& rOutputStream, const char* pFilename, bool isBinary = false, bool isAppend = false );
};
