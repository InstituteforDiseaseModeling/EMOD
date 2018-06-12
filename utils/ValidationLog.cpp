/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <fstream>
#include <string.h>
#include "ValidationLog.h"
#include "Environment.h"
#include "FileSystem.h"
#include "Log.h"

SETUP_LOGGING( "ValidationLog" )

ValidationLog::ValidationLog(std::ifstream *reference_file) // use to run in validate mode
: referenceInputFile(reference_file), isValidating(true), exitOnValidationFailure(false), isEnabled(true), _index_(0)
{ }

ValidationLog::ValidationLog(std::ofstream *reference_file) // use to create validation file
: referenceOutputFile(reference_file), isValidating(false), exitOnValidationFailure(false), isEnabled(true), _index_(0)
{ }

ValidationLog::ValidationLog()
: isEnabled(false)
{ }

void ValidationLog::Log( std::string s ) /* log message to record or to validate against */
{
    if (!isEnabled) return;

    char delim = '\n';
    if (isValidating)
    {
#define USE_FAST_COMPARISON 0
#if USE_FAST_COMPARISON // faster but less safe

        char our_string[4096];
        char ref_string[4096];

        referenceInputFile->getline(ref_string, 4095);

        sprintf(our_string, "%d: %s", _index_++, s.c_str());      

        if (0 != strcmp(our_string, ref_string))
        {
            LOG_INFO_F("[ValidationFailure] Test '%s' != Validation '%s'\n", our_string, ref_string);
            if (exitOnValidationFailure) exit(-1);        
        }
#else
        static boost::format _fmt_("%1%: %2%");

        std::string ref_string;
        std::string our_string((_fmt_ % _index_++ % s).str());
        getline(*referenceInputFile, ref_string); //, referenceInputFile->widen(delim)); // apparently there were some problems with MS's implementation of the stream library not accepting a delim here(?). maybe fix later

        if (0 != strcmp(our_string.c_str(), ref_string.c_str())) // for some reason operator != isnt working
        {
            LOG_INFO_F("[ValidationFailure] Test '%s' != Validation '%s'\n", our_string.c_str(), ref_string.c_str());
            if (exitOnValidationFailure) exit(-1);
        }
#endif
    }
    else
    {
        (*referenceOutputFile) << _index_++ << ": " << s << delim;
        (*referenceOutputFile).flush(); // temporary hack to allow stepping through code easily
    }

    if (_index_ == 5253)
        LOG_INFO("BREAK HERE");
}

ValidationLog* ValidationLog::Open( std::string filename, bool is_validating )
{
    ValidationLog* newlog = (ValidationLog*)0;

    if( is_validating )
    {
        if( FileSystem::FileExists( filename ) )
        {
            std::ifstream *ifs = _new_ std::ifstream();
            FileSystem::OpenFileForReading( *ifs, filename.c_str() );
            newlog = _new_ ValidationLog(ifs);
        }
    }
    else 
    {
        std::ofstream *ofs = _new_ std::ofstream();
        FileSystem::OpenFileForWriting( *ofs, filename.c_str() );
        newlog = _new_ ValidationLog(ofs);
    }

    CurrentLog = newlog;

    return newlog;
}

ValidationLog* ValidationLog::CreateNull()
{
    CurrentLog = _new_ ValidationLog();

    return CurrentLog;
}

ValidationLog* ValidationLog::CurrentLog = (ValidationLog*)0;
