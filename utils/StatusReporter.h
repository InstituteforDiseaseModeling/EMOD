/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <fstream>

#ifdef WIN32

#include <sstream>
#include "comutil.h"

// HPC Job Scheduler Support
struct IScheduler;
struct ISchedulerJob;

// UDP Listener Support
class LocalWinsockJunk;
#define STATUS_REPORTER_BUFSIZE 256

class StatusReporter
{
public:
    static StatusReporter * getInstance();
    static bool updateScheduler;
    static bool updateListener;

    ~StatusReporter(void);
    void ReportStatus(const std::string &status);
    void ReportProgress(int step, int steps);
    void ReportInitializationProgress(int pop, int total_pop);

    // UDP Listener Support
    void SetHost( const std::string& hostname );
    void SetMonitorPort( int port );
    void SetHPCId( const std::string& jobId );

private:
    StatusReporter(void);
    static StatusReporter *_instance;

    void OpenStatusFile();
    std::ofstream statusfile;
    void WriteStatusToFile( const std::string & status );

    // HPC Job Scheduler Support
    void TryConnectToJobScheduler();

    _bstr_t * GetSchedulerName();
    long GetJobId();
    void GetSchedulerInterface();
    void ConnectToScheduler( _bstr_t * schedulerName );
    void GetJobInterface( long jobId );
    void UpdateJobProgress( int step, int steps, const std::string &status );
    void UpdateJobProgressMessage( const std::string &status );
    void CommitStatusUpdate();
    void CleanupCom();

    static bool coInitialized;
    IScheduler *scheduler;
    ISchedulerJob *schedulerJob;

    std::ostringstream errorMessage;

    // UDP Listener Support
    void TryOpenUdpSocket();

    void AllocateLocalWinsockData();
    void ConfigureLocalWinsockData();
    void InitializeWinsock();
    void CreateSendSocket();
    void SendUdpStatusPacket( const std::string & status );

    void publish(const std::string &newLogLine) const;
    int m_nPort;
    std::string m_szHost;
    std::string m_szJobId;
    LocalWinsockJunk * m_pLocalWinsockJunk;
};

#else

class StatusReporter
{
public:
    ~StatusReporter(void);
    void ReportStatus(const std::string &status);
    void ReportProgress(int step, int steps);
    void ReportInitializationProgress(int pop, int total_pop);
    static StatusReporter * getInstance();
    static bool updateScheduler;
    static bool updateListener;

    // UDP Listener Support
    void SetHost( const std::string& hostname );
    void SetMonitorPort( int port );
    void SetHPCId( const std::string& jobId );

private:
    StatusReporter(void);

    static StatusReporter *_instance;

    std::ofstream statusfile;
    void WriteStatusToFile( const std::string & status );
};

#endif
