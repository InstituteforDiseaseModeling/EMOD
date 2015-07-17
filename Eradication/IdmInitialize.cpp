/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "IdmApi.h"

#include <map>

bool _processIsInitialized = false;
#ifdef _WIN32
std::map<DWORD, bool>* _threadMap;

extern "C" int __stdcall _CRT_INIT(_In_ void * _HDllHandle, _In_ unsigned _Reason, _In_opt_ void * _Reserved);

void IDMAPI IdmInitialize()
{
    DWORD threadId = GetThreadId(GetCurrentThread());

    if (!_processIsInitialized)
    {
        _CRT_INIT(GetModuleHandleA("eradication.exe"), DLL_PROCESS_ATTACH, NULL);
        _processIsInitialized = true;
        _threadMap = new std::map<DWORD, bool>();
    }

    if (!(*_threadMap)[threadId])
    {
        _CRT_INIT(GetModuleHandleA("eradication.exe"), DLL_THREAD_ATTACH, NULL);
        (*_threadMap)[threadId] = true;
    }
}
#endif
