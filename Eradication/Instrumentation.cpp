/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include <iostream>
#include "Instrumentation.h"
#include "Log.h"
#include "Debug.h"

static int MEMORY_PAGES_LIMIT = 50;

static const char * _module = "Instrumentation";

void * MemoryGauge::processHandle = NULL;

#ifndef WIN32
// dummy implementations for non-windows platforms
//

Stopwatch::Stopwatch( bool ) { }
void Stopwatch::Start() { }
void Stopwatch::Stop() { }
double Stopwatch::ResultNanoseconds() { return 0; }
void Stopwatch::PrintResultSeconds(const char * label) { }

void MemoryGauge::PrintMemoryUsage() { }
void MemoryGauge::PrintMemoryFree() { }
void MemoryGauge::CheckMemoryFailure() { }

#else

#include <windows.h>
#include <Psapi.h>

void MemoryGauge::PrintMemoryUsage()
{
    if(!processHandle)
    {
        processHandle = GetCurrentProcess();
    }
    release_assert(processHandle);

    PROCESS_MEMORY_COUNTERS_EX meminfo;
    if(GetProcessMemoryInfo(processHandle, (PROCESS_MEMORY_COUNTERS*)&meminfo, sizeof(meminfo)))
    {
        LOG_INFO_F("Working-set: %uMB\n", meminfo.WorkingSetSize >> 20);
        LOG_INFO_F("Peak working-set: %uMB\n", meminfo.PeakWorkingSetSize >> 20);
    }
    else
        LOG_WARN("Unable to get memory working-set\n");
}

void MemoryGauge::PrintMemoryFree()
{
    MEMORYSTATUSEX memstatus;
    memstatus.dwLength = sizeof(memstatus);

    if(GlobalMemoryStatusEx(&memstatus))
    {
        LOG_INFO_F("Physical memory load: %u%%\n", memstatus.dwMemoryLoad);
        LOG_INFO_F("Available physical memory: %uMB\n", memstatus.ullAvailPhys >> 20);
    }
    else
        LOG_WARN("Unable to get global memory\n");
}

void MemoryGauge::CheckMemoryFailure()
{

    int64_t pagefile_size = 0;
    if(!processHandle)
    {
        processHandle = GetCurrentProcess();
    }
    release_assert(processHandle);

    PROCESS_MEMORY_COUNTERS_EX meminfo;
    if(GetProcessMemoryInfo(processHandle, (PROCESS_MEMORY_COUNTERS*)&meminfo, sizeof(meminfo)))
    {
        pagefile_size = ((int64_t)meminfo.PagefileUsage) >> 20;
    }
    else
        LOG_WARN("Unable to get memory pagefile\n");

    // TBD: these two number are constants therefore only needed to be computed once
    MEMORYSTATUSEX memstatus;
    memstatus.dwLength = sizeof(memstatus);
    int64_t total_pmem_size = 0;
    int64_t total_vm_size = 0;
    int64_t vm_pmem_delta = 0;
    if(GlobalMemoryStatusEx(&memstatus))
    {
        total_pmem_size = ((int64_t)memstatus.ullTotalPhys) >> 20;
        total_vm_size = ((int64_t)memstatus.ullTotalPageFile) >> 20;
        
        // This calculation is subjectively reasonable to not overrun the VM too much
        // so that the exception bad_alloc can be caught in time, to be optimized later
        vm_pmem_delta = total_vm_size > total_pmem_size ? ((total_vm_size - total_pmem_size) >> 2) + total_pmem_size 
                                                        : total_pmem_size + (total_pmem_size >> 1);
    }
    else
    {
        LOG_WARN("Unable to get  memory\n");
    }

    static int cnt = 0;
    if (pagefile_size > vm_pmem_delta)
    {

        LOG_WARN_F("Runs out memory %d times: physical memory: %u MB, VM limit %u MB, total pagedfile size %u MB\n", cnt, total_pmem_size 
                                                                                , vm_pmem_delta, pagefile_size);
        if (++cnt == MEMORY_PAGES_LIMIT)
        {
            LOG_ERR("Application is exiting.  This may take a long time.  You might want to consider killing the task.\n");
            throw std::bad_alloc();
        }
    }
    else if (pagefile_size < total_pmem_size)
    {
        // reset back for re-accumulating
        cnt = 0;
    }
    
}

//////////////////////////////////////////////////////////////////////////
// adapted from http://stackoverflow.com/questions/922829/c-boost-posix-time-elapsed-seconds-elapsed-fractional-seconds

//! \brief Stopwatch for timing performance values
//!
//! This stopwatch class is designed for timing performance of various
//! software operations.  If the values you get back a greater than a 
//! few seconds, you should be using a different tool.
//! On a Core 2 Duo E6850 @ 3.00GHz, the start/stop sequence takes 
//! approximately 230 nano seconds in the debug configuration and 180 
//! nano seconds in the release configuration.  If you are timing on the
//! sub-microsecond scale, please take this into account and test it on
//! your machine.


//! \param start if set to true will initialize and then start the 
//! timer.
Stopwatch::Stopwatch(bool start)
{
    _start = 0;
    _stop = 0;
    if(start)
        Start();
}

//! Starts the stopwatch running
void Stopwatch::Start()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&_start);
}

//! Run this when the event being timed is complete
void Stopwatch::Stop()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&_stop);
}

//! Stops the timer and returns the result
//double Stopwatch::StopResult()
//{
//    Stop();
//    return ResultNanoseconds();
//}

//! You can get the result of the stopwatch start-stop sequence at
//! your leisure.
double Stopwatch::ResultNanoseconds()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double cyclesPerNanosecond = static_cast<double>(frequency.QuadPart) / 1000000000.0;

    uint64_t elapsed = _stop - _start;
    return elapsed / cyclesPerNanosecond;
}

void Stopwatch::PrintResultSeconds(const char * label)
{
    LOG_INFO_F("%s - %f secs\n", label, ResultNanoseconds() / 1000000000);
}

#endif
