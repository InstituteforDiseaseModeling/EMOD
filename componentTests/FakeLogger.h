
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
