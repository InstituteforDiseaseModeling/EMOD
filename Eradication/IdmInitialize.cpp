/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "IdmApi.h"

#include <map>

bool _processIsInitialized = false;

#if _WIN32 && !DISABLE_MPI //DISABLE_MPI is also implying static runtime which means we can't treat EXE as DLL.

std::map<DWORD, bool>* _threadMap;

extern "C" int __stdcall _CRT_INIT(_In_ void * _HDllHandle, _In_ unsigned _Reason, _In_opt_ void * _Reserved);

void IDMAPI IdmInitialize()
{
    DWORD threadId = GetThreadId(GetCurrentThread());

    if (!_processIsInitialized)
    {
        _CRT_INIT(GetModuleHandleA("eradication.exe"), DLL_PROCESS_ATTACH, nullptr);
        _processIsInitialized = true;
        _threadMap = new std::map<DWORD, bool>();
    }

    if (!(*_threadMap)[threadId])
    {
        _CRT_INIT(GetModuleHandleA("eradication.exe"), DLL_THREAD_ATTACH, nullptr);
        (*_threadMap)[threadId] = true;
    }
}
#else
void IDMAPI IdmInitialize()
{
}
#endif
