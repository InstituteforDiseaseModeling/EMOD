/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <stdarg.h>
#include <iostream>
#include "Environment.h"
#include "Exceptions.h"
#include <map>
#include "FakeLogger.h"

using namespace std;
#pragma warning(disable : 4996)

SETUP_LOGGING("FakeLogger")

void
FakeLogger::Log(
    Logger::tLevel log_level,
    const char* module,
    const char* msg, ...)
{
    // Translate this variadically-constructed message into a single string for storage
    // Magic courtesy of http://www.cplusplus.com/forum/general/133535/
    va_list args, args_copy;
    va_start(args, msg);
    va_copy(args_copy, args);

    const auto sz = std::vsnprintf(nullptr, 0, msg, args) + 1;
    std::string result(sz, ' ');
    std::vsnprintf(&result.front(), sz, msg, args_copy);

    va_end(args_copy);
    va_end(args);

    // Construct the log message
    LogEntry newEntry;
    newEntry.log_level = log_level;
    newEntry.module = module;
    newEntry.msg = result;

    // Store the log message
    _vector.push_back(newEntry);
}

void FakeLogger::Flush()
{
    _vector.clear();
}

bool FakeLogger::Empty() const
{
    return _vector.empty();
}

const LogEntry& FakeLogger::Back() const
{
    return _vector.back();
}

void FakeLogger::Pop_Back()
{
    _vector.pop_back();
}