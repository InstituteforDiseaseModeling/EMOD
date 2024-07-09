
#include "stdafx.h"

#include <iostream>
#include "Memory.h"
#include "Log.h"
#include "Debug.h"

#ifdef WIN32

#include <windows.h>
#include <Psapi.h>

const char* CURRENT_NAME       = "Working-Set              ";
const char* PEAK_NAME          = "Peak Working-Set         ";
const char* VIRTUAL_NAME       = "Pagefile Usage           ";
const char* PEAK_VIRTUAL_NAME  = "Peak Pagefile Usage      ";
const char* RAM_FREE_NAME      = "Available Physical Memory";
const char* RAM_TOTAL_NAME     = "Total Physical Memory    ";
const char* VIRTUAL_FREE_NAME  = "Available Page File      ";
const char* VIRTUAL_TOTAL_NAME = "Total Page File          ";

#else

#include <linux/kernel.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <istream>

const char* CURRENT_NAME       = "Resident Set Size (VmRSS)        ";
const char* PEAK_NAME          = "Peak resident set size (VmHWM)   ";
const char* VIRTUAL_NAME       = "Virtual memory size (VmSize)     ";
const char* PEAK_VIRTUAL_NAME  = "Peak virtual memory size (VmPeak)";
const char* RAM_FREE_NAME      = "RAM not in use (MemFree)         ";
const char* RAM_TOTAL_NAME     = "Total usable RAM (MemTotal)      ";
const char* VIRTUAL_FREE_NAME  = "Unused Swap Space (SwapFree)     ";
const char* VIRTUAL_TOTAL_NAME = "Swap Space Available (SwapTotal) ";

#endif

SETUP_LOGGING( "Memory" )

namespace Kernel
{

    GET_SCHEMA_STATIC_WRAPPER_IMPL( MemoryGauge, MemoryGauge )

    MemoryGauge::MemoryGauge()
        : JsonConfigurable()
        , m_ProcessHandle( nullptr )
        , m_WorkingSetWarningMB( 7000 )
        , m_WorkingSetHaltMB( 8000 )
        , m_LastPeakSizeMB( 7000 )
        , m_GettingDataFailureOccurred( false )
        , m_ProcessMemory()
        , m_SystemMemory()
    {
    }

    MemoryGauge::~MemoryGauge()
    {
    }

    bool MemoryGauge::Configure( const Configuration* inputJson )
    {
        if( JsonConfigurable::_dryrun || inputJson->Exist( "Memory_Usage_Warning_Threshold_Working_Set_MB" ) )
        {
            initConfigTypeMap( "Memory_Usage_Warning_Threshold_Working_Set_MB", (int*)&m_WorkingSetWarningMB, Memory_Usage_Warning_Threshold_Working_Set_MB_DESC_TEXT, 0, 1000000, 7000 );
        }

        if( JsonConfigurable::_dryrun || inputJson->Exist( "Memory_Usage_Halting_Threshold_Working_Set_MB" ) )
        {
            initConfigTypeMap( "Memory_Usage_Halting_Threshold_Working_Set_MB", (int*)&m_WorkingSetHaltMB, Memory_Usage_Halting_Threshold_Working_Set_MB_DESC_TEXT, 0, 1000000, 8000 );
        }

        bool retValue = JsonConfigurable::Configure( inputJson );
        if( retValue && !JsonConfigurable::_dryrun )
        {
            if( m_WorkingSetHaltMB < m_WorkingSetWarningMB )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                    "Memory_Usage_Warning_Threshold_Working_Set_MB", (unsigned long)(m_WorkingSetWarningMB),
                    "Memory_Usage_Halting_Threshold_Working_Set_MB", (unsigned long)(m_WorkingSetHaltMB),
                    "\nThe Warning WorkingSet threshold must be smaller than the Halting WorkingSet threshold." );
            }
            m_LastPeakSizeMB = m_WorkingSetWarningMB;
        }
        return retValue;
    }

    void MemoryGauge::CheckMemoryFailure( bool onlyCheckForFailure )
    {
        if( m_GettingDataFailureOccurred ) return;

        const ProcessMemoryInfo& r_proc_mem = GetProcessMemory();
        if( !r_proc_mem.success )
        {
            LOG_WARN( "Unable to get process memory information.  No checking will be done.\n" );
            m_GettingDataFailureOccurred = true;
            return;
        }

        const SystemMemoryInfo&  r_sys_mem  = GetSystemMemory();
        if( !r_sys_mem.success )
        {
            LOG_WARN( "Unable to get system memory.  No checking will be done.\n" );
            m_GettingDataFailureOccurred = true;
            return;
        }

        Logger::tLevel log_level = Logger::DEBUG;
        bool log_data = EnvPtr->Log->IsLoggingEnabled( log_level, _module, _log_level_enabled_array );
        if( m_WorkingSetWarningMB <= r_proc_mem.currentMB )
        {
            log_data = true;
            log_level = Logger::WARNING;
        }
        else if( m_LastPeakSizeMB < r_proc_mem.peakCurrentMB )
        {
            log_data = true;
            log_level = Logger::WARNING;
            m_LastPeakSizeMB = r_proc_mem.peakCurrentMB;
        }

        if( onlyCheckForFailure && (log_level == Logger::DEBUG) )
        {
            log_data = false;
        }

        if( log_data )
        {
            EnvPtr->Log->Log( log_level, _module, "%s: %uMB\n", CURRENT_NAME,      r_proc_mem.currentMB     );
            EnvPtr->Log->Log( log_level, _module, "%s: %uMB\n", PEAK_NAME,         r_proc_mem.peakCurrentMB );
            EnvPtr->Log->Log( log_level, _module, "%s: %uMB\n", VIRTUAL_NAME,      r_proc_mem.virtualMB     );
            EnvPtr->Log->Log( log_level, _module, "%s: %uMB\n", PEAK_VIRTUAL_NAME, r_proc_mem.peakVirtualMB );

            EnvPtr->Log->Log( log_level, _module, "%s: %uMB\n", RAM_FREE_NAME,      r_sys_mem.ramFreeMB      );
            EnvPtr->Log->Log( log_level, _module, "%s: %uMB\n", RAM_TOTAL_NAME,     r_sys_mem.ramTotalMB     );
            EnvPtr->Log->Log( log_level, _module, "%s: %uMB\n", VIRTUAL_FREE_NAME,  r_sys_mem.virtualFreeMB  );
            EnvPtr->Log->Log( log_level, _module, "%s: %uMB\n", VIRTUAL_TOTAL_NAME, r_sys_mem.virtualTotalMB );
        }

        if( m_WorkingSetHaltMB <= r_proc_mem.currentMB )
        {
            std::stringstream ss;
            ss << "Current memory usage (" << CURRENT_NAME << " = " << r_proc_mem.currentMB << " MB) exceeds limit of " << m_WorkingSetHaltMB << " MB.\n";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

#ifdef WIN32

    const ProcessMemoryInfo& MemoryGauge::GetProcessMemory()
    {
        if( m_ProcessHandle == nullptr )
        {
            m_ProcessHandle = GetCurrentProcess();
        }
        release_assert( m_ProcessHandle );

        m_ProcessMemory.currentMB     = 0;
        m_ProcessMemory.peakCurrentMB = 0; 
        m_ProcessMemory.virtualMB     = 0;
        m_ProcessMemory.peakVirtualMB = 0;
        m_ProcessMemory.success       = false;

        PROCESS_MEMORY_COUNTERS_EX meminfo;
        if( GetProcessMemoryInfo( m_ProcessHandle, (PROCESS_MEMORY_COUNTERS*)&meminfo, sizeof( meminfo ) ) )
        {
            // convert from bytes to MB
            m_ProcessMemory.currentMB     = meminfo.WorkingSetSize     >> 20;
            m_ProcessMemory.peakCurrentMB = meminfo.PeakWorkingSetSize >> 20;
            m_ProcessMemory.virtualMB     = meminfo.PagefileUsage      >> 20;
            m_ProcessMemory.peakVirtualMB = meminfo.PeakPagefileUsage  >> 20;
            m_ProcessMemory.success = true;
        }

        return m_ProcessMemory;
    }

    const SystemMemoryInfo& MemoryGauge::GetSystemMemory()
    {
        m_SystemMemory.ramFreeMB      = 0;
        m_SystemMemory.ramTotalMB     = 0;
        m_SystemMemory.virtualFreeMB  = 0;
        m_SystemMemory.virtualTotalMB = 0;
        m_SystemMemory.success        = false;

        MEMORYSTATUSEX memstatus;
        memstatus.dwLength = sizeof( memstatus );

        if( GlobalMemoryStatusEx( &memstatus ) )
        {
            // convert from bytes to MB
            m_SystemMemory.ramFreeMB      = memstatus.ullAvailPhys     >> 20;
            m_SystemMemory.ramTotalMB     = memstatus.ullTotalPhys     >> 20;
            m_SystemMemory.virtualFreeMB  = memstatus.ullAvailPageFile >> 20;
            m_SystemMemory.virtualTotalMB = memstatus.ullTotalPageFile >> 20;
            m_SystemMemory.success = true;
        }
        return m_SystemMemory;
    }

#else

    int GetValueFromLine( char* line )
    {
        // This assumes that a digit will be found and the line ends in " Kb".
        int i = strlen(line);
        const char* p = line;
        while (*p <'0' || *p > '9') p++;
        line[i-3] = '\0';

        int val = atoi(p);
        return val;
    }

    int GetValue( ifstream& rStream, const char* name, int nameSize )
    {
        int val = -1;
        bool found = false;
        int num_lines = 0;
        while( !rStream.eof() && rStream.good() )
        {
            static char line[ 1000 ];
            rStream.getline( line, 1000 );
            if( strncmp( line, name, nameSize ) == 0)
            {
                val = GetValueFromLine( line );
                found = true;
                break;
            }
            // protection against endless loop
            ++num_lines;
            release_assert( num_lines <= 50 );
        }
        if( !found )
        {
            LOG_WARN_F( "Did not find '%s' in '/proc/self/status'.\n", name );
        }

        return val;
    }

    const ProcessMemoryInfo& MemoryGauge::GetProcessMemory()
    {
        m_ProcessMemory.currentMB     = 0;
        m_ProcessMemory.peakCurrentMB = 0; 
        m_ProcessMemory.virtualMB     = 0;
        m_ProcessMemory.peakVirtualMB = 0;
        m_ProcessMemory.success       = false;

        ifstream stream( "/proc/self/status", ios_base::in );
        if( stream.good() )
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! Order is important.  This is the order the values appear in the /proc/self/status file
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            int peak_virtual_kb = GetValue( stream, "VmPeak", 6 );
            int virtual_kb      = GetValue( stream, "VmSize", 6 );
            int peak_current_kb = GetValue( stream, "VmHWM" , 5 );
            int current_kb      = GetValue( stream, "VmRSS" , 5 );

            if( (peak_virtual_kb >= 0) && (virtual_kb >= 0) && (peak_current_kb >= 0) && (current_kb >= 0) )
            {
                // Convert from kB to MB
                m_ProcessMemory.currentMB      = uint32_t(current_kb     ) >> 10;
                m_ProcessMemory.peakCurrentMB  = uint32_t(peak_current_kb) >> 10;
                m_ProcessMemory.virtualMB      = uint32_t(virtual_kb     ) >> 10;
                m_ProcessMemory.peakVirtualMB  = uint32_t(peak_virtual_kb) >> 10;
                m_ProcessMemory.success = true;
            }
        }
        stream.close();

        return m_ProcessMemory;
    }

    const SystemMemoryInfo& MemoryGauge::GetSystemMemory()
    {
        m_SystemMemory.ramFreeMB      = 0;
        m_SystemMemory.ramTotalMB     = 0;
        m_SystemMemory.virtualFreeMB  = 0;
        m_SystemMemory.virtualTotalMB = 0;
        m_SystemMemory.success        = false;

        struct sysinfo sys_info;
        memset( &sys_info, 0, sizeof( struct sysinfo ) );

        if( sysinfo( &sys_info ) == 0 )
        {
            m_SystemMemory.ramFreeMB      = sys_info.freeram   >> 20;
            m_SystemMemory.ramTotalMB     = sys_info.totalram  >> 20;
            m_SystemMemory.virtualFreeMB  = sys_info.freeswap  >> 20;
            m_SystemMemory.virtualTotalMB = sys_info.totalswap >> 20;
            m_SystemMemory.success = true;
        }

        return m_SystemMemory;
    }
#endif
}
