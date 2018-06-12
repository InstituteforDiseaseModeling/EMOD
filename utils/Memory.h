/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"

namespace Kernel
{
    class MemoryGauge : public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER( MemoryGauge )
    public:

        static void CheckMemoryFailure( bool onlyCheckForFailure );

        MemoryGauge();
        virtual ~MemoryGauge();

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) { return e_NOINTERFACE; }
        virtual int32_t AddRef() { return -1; }
        virtual int32_t Release() { return -1; }

    private:
        static void*    m_ProcessHandle;
        static uint64_t m_WorkingSetWarningMB;
        static uint64_t m_WorkingSetHaltMB;
        static uint64_t m_LastPeakSizeMB;
    };
}