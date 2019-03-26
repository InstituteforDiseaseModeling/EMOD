/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include "stdafx.h"
#include "IdmApi.h"
#include "Sugar.h"
#include "Log.h"

struct LogEntry
{
    Logger::tLevel log_level;
    std::string module;
    std::string msg;
};

class IDMAPI FakeLogger : public SimpleLogger
{
public:
    FakeLogger() : SimpleLogger() {}
    FakeLogger(Logger::tLevel syslevel) : SimpleLogger(syslevel) {}

    virtual void Log(Logger::tLevel log_level, const char* module, const char* msg, ...) override;
    virtual void Flush() override;

    bool Empty() const;
    const LogEntry& Back() const;
    void Pop_Back();
    
private:
    std::vector<LogEntry> _vector;
};
