
#pragma once

#include "Configure.h"

namespace Kernel
{
    struct ProcessMemoryInfo
    {
        uint32_t currentMB;     // Resident set size (Linux) / Working-set (Windows)
        uint32_t peakCurrentMB; // Peak resident set size ("high water mark") (Linux) / Peak working-set (Windows
        uint32_t virtualMB;     // Virtual memory size (linux) / Pagefile Usage (Windows)
        uint32_t peakVirtualMB; // Peak virtual memory size (linux) / Peak Pagefile Usage (Windows)
        bool     success;

        ProcessMemoryInfo()
            : currentMB(0)
            , peakCurrentMB(0)
            , virtualMB(0)
            , peakVirtualMB(0)
            , success(false)
        {
        }
    };

    struct SystemMemoryInfo
    {
        uint32_t ramFreeMB;      // The sum of LowFree+HighFree (linux) / Available physical memory (Windows)
        uint32_t ramTotalMB;     // Total usable RAM (i.e., physical RAM minus a few reserved bits and the kernel binary code). / Total physical memory  (windows)
        uint32_t virtualFreeMB;  // Amount of swap space that is currently unused.(linux) / Available Page File (Windows)
        uint32_t virtualTotalMB; // Total amount of swap space available. (linux) / Total Page File (Windows)
        bool     success;

        SystemMemoryInfo()
            : ramFreeMB( 0 )
            , ramTotalMB( 0 )
            , virtualFreeMB( 0 )
            , virtualTotalMB( 0 )
            , success( false )
        {
        }
    };



    class MemoryGauge : public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER( MemoryGauge )
    public:
        MemoryGauge();
        virtual ~MemoryGauge();

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) { return e_NOINTERFACE; }
        virtual int32_t AddRef() { return -1; }
        virtual int32_t Release() { return -1; }

        void CheckMemoryFailure( bool onlyCheckForFailure );

        const ProcessMemoryInfo& GetProcessMemory();
        const SystemMemoryInfo& GetSystemMemory();

    private:
        void*    m_ProcessHandle;
        uint64_t m_WorkingSetWarningMB;
        uint64_t m_WorkingSetHaltMB;
        uint64_t m_LastPeakSizeMB;
        bool     m_GettingDataFailureOccurred;

        ProcessMemoryInfo m_ProcessMemory;
        SystemMemoryInfo  m_SystemMemory;
    };
}