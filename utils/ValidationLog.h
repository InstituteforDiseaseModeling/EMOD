/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "BoostLibWrapper.h"

class ValidationLog
{
public:

    static ValidationLog* Open(std::string filename, bool is_validating);
    static ValidationLog* CreateNull();

    static ValidationLog* CurrentLog;

    // tagged logging options to help zoom in on specific sections of code
    virtual void EnableTag(std::string tag, bool state) { enables[tag] = state ? 1 : 0; }

    virtual void SetExitOnValidationFailure(bool exit_on_failure) { exitOnValidationFailure = exit_on_failure; } // useful for automated test scripts to avoid having tons of useless spew
    virtual void Log(std::string tag, boost::format &fmt)
    {
        if (isEnabled && enables[tag])
            Log(fmt);
    }

    virtual void Enable(bool enable) { isEnabled = enable; }
    virtual bool IsEnabled() { return isEnabled; }

    virtual void Log(boost::format &format) // helper so we can accept boost format somewhat concisely
    {
        if (isEnabled) Log(boost::str(format));
    }

    virtual void Log(std::string s); // log message to record or to validate against;

    virtual ~ValidationLog(void) 
    { 
        if (isEnabled)
        {
            if (isValidating)
            {
                referenceInputFile->close();
                delete referenceInputFile;
            }
            else
            {
                referenceOutputFile->close();
                delete referenceOutputFile;
            }
        }

        CurrentLog = (ValidationLog*)0;
    }

protected:
    std::map<std::string,int> enables;

    bool isEnabled;
    bool isValidating;
    bool exitOnValidationFailure;
    std::ifstream *referenceInputFile;
    std::ofstream *referenceOutputFile;
    unsigned int _index_;

private: 
    ValidationLog(std::ifstream *reference_file); // use to run in validate mode
    ValidationLog(std::ofstream *reference_file); // use to create validation file
    ValidationLog();
};

#define VALIDATE(_msg)  { if (ValidationLog::CurrentLog->IsEnabled()) ValidationLog::CurrentLog->Log(_msg); }
