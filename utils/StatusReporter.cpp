/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/
#include "stdafx.h"
#include "Debug.h"
#include "StatusReporter.h"
#include "FileSystem.h"
#include <sstream>

// Don't force upgrade of gcc just for this. Actual ifdef isn't really win32, it's c++11, but nullptr requires gcc 4.6
#ifdef WIN32
StatusReporter *StatusReporter::_instance = nullptr;
#else
StatusReporter *StatusReporter::_instance = __null;
#endif

StatusReporter * StatusReporter::getInstance()
{
    return _instance ? _instance : _instance = _new_ StatusReporter;
}

void StatusReporter::WriteStatusToFile( const std::string & status )
{
    statusfile << status << std::endl;
    statusfile.flush();
}

#ifdef WIN32

/*
// #import is incompatible with multi-threaded build.
// The solution is to use the generated .tlh files directly, as below
#import <Microsoft.Hpc.Scheduler.tlb> named_guids no_namespace raw_interfaces_only \
    rename("SetEnvironmentVariable","SetHpcEnvironmentVariable") \
    rename("AddJob", "AddHpcJob")
#import <Microsoft.Hpc.Scheduler.Properties.tlb> named_guids no_namespace raw_interfaces_only 
*/
#include "microsoft.hpc.scheduler.tlh"
#include "microsoft.hpc.scheduler.properties.tlh"

#include "log.h"

#include <stdio.h>
#include <winsock.h>

// Required for winsock.h and winsock2.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

SETUP_LOGGING( "StatusReporter" )

// Link with Ws2_32.lib to resolve these types
class LocalWinsockJunk
{
public:
    WSADATA wsaData;
    SOCKET SendSocket;
    sockaddr_in RecvAddr;
};

bool StatusReporter::updateScheduler = false;
bool StatusReporter::updateListener  = false;
bool StatusReporter::coInitialized = false;

time_t lastProgressUpdateTime = 0;

StatusReporter::StatusReporter(void) :
    scheduler(nullptr),
    schedulerJob(nullptr),
    m_nPort(4444),
    m_pLocalWinsockJunk(nullptr)
{
    LOG_DEBUG("Beginning to construct StatusReporter...\n");

    OpenStatusFile();
    if (updateScheduler) TryConnectToJobScheduler();
    if (updateListener)  TryOpenUdpSocket();

    LOG_DEBUG("...finished constructing StatusReporter.\n");
}

void StatusReporter::OpenStatusFile()
{
    FileSystem::OpenFileForWriting( statusfile, "status.txt" );
}

void StatusReporter::TryConnectToJobScheduler()
{
    _bstr_t *schedulerName = nullptr;

    try
    {
        schedulerName = GetSchedulerName();
        long jobId    = GetJobId();
        GetSchedulerInterface();
        ConnectToScheduler(schedulerName);
        GetJobInterface(jobId);
    }
    catch (char *msg)
    {
        LOG_WARN(msg);
    }

    delete schedulerName;
}

_bstr_t * StatusReporter::GetSchedulerName()
{
    char *schedulerNameBuffer = nullptr;
    size_t bytesAllocated     = 0;
    // FYI - some examples say you should get the "CCP_HEAD_NODE" environment variable,
    // if you do this and don't have the correct permissions, you will hard hang right here.
    int retval                = _dupenv_s(&schedulerNameBuffer, &bytesAllocated, "CCP_SCHEDULER");

    if ((retval != 0) || (bytesAllocated == 0))
    {
        throw "Couldn't get scheduler name from 'CCP_SCHEDULER' environment variable.\n";
    }

    _bstr_t *schedulerName = new _bstr_t(schedulerNameBuffer);
    free(schedulerNameBuffer);

    if (!schedulerName)
    {
        throw "Couldn't create BSTR for schedulerName - out of memory?\n";
    }

    return schedulerName;
}

long StatusReporter::GetJobId()
{
    char *jobIdBuffer     = nullptr;
    size_t bytesAllocated = 0;
    int retval            = _dupenv_s(&jobIdBuffer, &bytesAllocated, "CCP_JOBID");

    if ((retval != 0) || (bytesAllocated == 0))
    {
        throw "Couldn't get job id 'CCP_JOBID' environment variable.\n";
    }

    long jobId = atoi(jobIdBuffer);
    free(jobIdBuffer);

    return jobId;
}

void StatusReporter::GetSchedulerInterface()
{
    HRESULT hr;
    if (FAILED(hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
    {
        errorMessage << "CoInitializeEx() failed - " << hr << std::endl;
        throw errorMessage.str().c_str();
    }

    coInitialized = true;

    hr = CoCreateInstance(
        __uuidof(Scheduler),
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IScheduler),
        reinterpret_cast<void **>(&scheduler));
    if (FAILED(hr))
    {
        errorMessage << "CoCreateInstance(Scheduler) failed - " << hr << std::endl;
        throw errorMessage.str().c_str();
    }
}

void StatusReporter::ConnectToScheduler( _bstr_t * schedulerName )
{
    HRESULT hr;
    if (FAILED(hr = scheduler->Connect(schedulerName->GetBSTR())))
    {
        errorMessage << "Couldn't connect to HPC scheduler '" << (char *)schedulerName << "' - " << hr << std::endl;
        throw errorMessage.str().c_str();
    }
}

void StatusReporter::GetJobInterface( long jobId )
{
    HRESULT hr;
    if (FAILED(hr = scheduler->OpenJob(jobId, &schedulerJob)))
    {
        errorMessage << "Couldn't open scheduler job " << jobId << " - " << hr << std::endl;
        throw errorMessage.str().c_str();
    }
}

void StatusReporter::TryOpenUdpSocket()
{
    try
    {
        AllocateLocalWinsockData();
        ConfigureLocalWinsockData();
        InitializeWinsock();
        CreateSendSocket();
    }
    catch (char *msg)
    {
        delete m_pLocalWinsockJunk;
        m_pLocalWinsockJunk = nullptr;
        LOG_WARN(msg);
    }
}

void StatusReporter::AllocateLocalWinsockData()
{
    m_pLocalWinsockJunk = _new_ LocalWinsockJunk;
    if (!m_pLocalWinsockJunk)
    {
        throw "Couldn't allocate WinSock data - out of memory?\n";
    }
}

void StatusReporter::ConfigureLocalWinsockData()
{
    m_pLocalWinsockJunk->RecvAddr.sin_family = AF_INET;
}

void StatusReporter::InitializeWinsock()
{
    int retval = WSAStartup(MAKEWORD(2,2), &(m_pLocalWinsockJunk->wsaData));
    if (retval != 0)
    {
        errorMessage << "WSAStartup() failed - " << retval << std::endl;
        throw errorMessage.str().c_str();
    }
}

void StatusReporter::CreateSendSocket()
{
    m_pLocalWinsockJunk->SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_pLocalWinsockJunk->SendSocket == INVALID_SOCKET)
    {
        int error = WSAGetLastError();
        errorMessage << "Error opening UDP socket for status reporting - " << error << std::endl;
        throw errorMessage.str().c_str();
    }
}

#define DELETEOBJ(object)  if (object) { delete object; object = nullptr; }
#define RELEASE(object) if (object) { object->Release(); object = nullptr; }

StatusReporter::~StatusReporter(void)
{
    DELETEOBJ(m_pLocalWinsockJunk);
    RELEASE(schedulerJob);
    RELEASE(scheduler);
    CleanupCom();
    statusfile.close();
}

void StatusReporter::CleanupCom()
{
    if (coInitialized)
    {
        CoUninitialize();
    }
}

void StatusReporter::ReportStatus(const std::string &status)
{
    WriteStatusToFile(status);
    lastProgressUpdateTime = 0;
    UpdateJobProgressMessage(status);
    SendUdpStatusPacket(status);
}

void StatusReporter::ReportProgress(int step, int steps)
{
    std::ostringstream messageStream;
    messageStream << step << " of " << steps << " steps complete.";

    WriteStatusToFile(messageStream.str());
    UpdateJobProgress(step, steps, messageStream.str());
    SendUdpStatusPacket(messageStream.str());
}

void StatusReporter::ReportInitializationProgress(int pop, int total_pop)
{
    std::ostringstream messageStream;
    messageStream << pop << " of " << total_pop << " (node) individuals initialized.";

    WriteStatusToFile(messageStream.str());
    UpdateJobProgressMessage(messageStream.str());
    SendUdpStatusPacket(messageStream.str());
}

#define SECONDS_BETWEEN_UPDATES 5

void StatusReporter::UpdateJobProgress( int step, int steps, const std::string &status )
{
    if (schedulerJob)
    {
        time_t now = time(nullptr);
        if(now - lastProgressUpdateTime >= SECONDS_BETWEEN_UPDATES || step == steps )
        {
            lastProgressUpdateTime = now;

            int percentage = (step * 100) / steps;
            schedulerJob->put_Progress(percentage);

            _bstr_t *progressMessage = new _bstr_t(status.c_str());
            schedulerJob->put_ProgressMessage(progressMessage->GetBSTR());

            CommitStatusUpdate();
            delete progressMessage;
        }
    }
}

void StatusReporter::UpdateJobProgressMessage( const std::string &status )
{
    if (schedulerJob)
    {
        time_t now = time(nullptr);
        if(now - lastProgressUpdateTime >= SECONDS_BETWEEN_UPDATES)
        {
            lastProgressUpdateTime = now;

            _bstr_t *progressMessage = new _bstr_t(status.c_str());
            schedulerJob->put_ProgressMessage(progressMessage->GetBSTR());

            CommitStatusUpdate();
            delete progressMessage;
        }
    }
}

void StatusReporter::CommitStatusUpdate() 
{
    HRESULT hr = schedulerJob->Commit();
    if (FAILED(hr))
    {
        std::ostringstream errorMessage;
        if (hr == 0x8004020B)
        {
            errorMessage << "Insufficient permissions to update progress information with the job scheduler." << std::endl;
        }
        else
        {
            errorMessage << "ISchedulerJob->Commit() failed - " << hr << std::endl;
        }
        LOG_WARN(errorMessage.str().c_str());

        // This will prevent continuing to try and update and receiving multiple warnings.
        RELEASE(schedulerJob);
        RELEASE(scheduler);
    }
}

void StatusReporter::SendUdpStatusPacket( const std::string & status )
{
    if (m_pLocalWinsockJunk)
    {
        std::ostringstream xmlyMsg;
        xmlyMsg << "<EmodKernelPacket>"
            << "<Source>"
            << m_szJobId 
            << "</Source>"
            << "<Message>"
            << status
            << "</Message>"
            << "</EmodKernelPacket>";
        publish( xmlyMsg.str() );
    }
}

void StatusReporter::publish(const std::string &newLogLine) const
{
    sendto(m_pLocalWinsockJunk->SendSocket,
        newLogLine.c_str(),
        int(strlen( newLogLine.c_str() )),
        0,
        (SOCKADDR *) &(m_pLocalWinsockJunk->RecvAddr),
        sizeof(m_pLocalWinsockJunk->RecvAddr));
}

void StatusReporter::SetHost( const std::string& hostname )
{
    updateListener = true;
    m_szHost                                      = hostname;
    if( !m_pLocalWinsockJunk )
    {
        TryOpenUdpSocket();
    }
    release_assert(m_pLocalWinsockJunk);
    m_pLocalWinsockJunk->RecvAddr.sin_addr.s_addr = inet_addr(m_szHost.c_str());
}

void StatusReporter::SetMonitorPort( int port )
{
    updateListener = true;
    m_nPort                                = port;
    if( !m_pLocalWinsockJunk )
    {
        TryOpenUdpSocket();
    }
    release_assert(m_pLocalWinsockJunk);
    m_pLocalWinsockJunk->RecvAddr.sin_port = htons(m_nPort);
}

void StatusReporter::SetHPCId( const std::string& jobId )
{
    updateListener = true;
    m_szJobId = jobId;
}

#else

bool StatusReporter::updateScheduler = false;
bool StatusReporter::updateListener  = false;

StatusReporter::StatusReporter(void)
{
    FileSystem::OpenFileForWriting( statusfile, "status.txt" );
}

StatusReporter::~StatusReporter(void)
{
    statusfile.close();
}

void StatusReporter::ReportStatus(const std::string &status)
{
    WriteStatusToFile(status);
}

void StatusReporter::ReportProgress(int step, int steps)
{
    std::ostringstream progressMessage;
    progressMessage << step << " of " << steps << " steps complete.";
    ReportStatus(progressMessage.str());
}

void StatusReporter::ReportInitializationProgress(int pop, int total_pop)
{
    std::ostringstream messageStream;
    messageStream << pop << " of " << total_pop << " (node) individuals initialized.";

    // This gets you file-based progress reporting
    WriteStatusToFile(messageStream.str());
    // This gets you network-based reporting over UDP
    ReportStatus(messageStream.str());
}

void StatusReporter::SetHost(const std::string& hostname) {}
void StatusReporter::SetMonitorPort(int port) {}
void StatusReporter::SetHPCId(const std::string& jobId) {}

#endif
