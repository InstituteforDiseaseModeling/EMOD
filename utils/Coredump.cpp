/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/
#include "stdafx.h"

#include <iostream>
#include <streambuf>
#include <map>
#include <string.h>

#ifdef WIN32
#include <Windows.h>
#include <DbgHelp.h>
#endif

#include "Coredump.h"

#pragma warning(disable : 4996)

#ifdef WIN32_LATER
const static int g_bufsize = sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR);

void CaptureCallstack(char* buf)
{

    PVOID bt[100];
    DWORD dwHashCode = 0;

    typedef USHORT (WINAPI *CaptureStackBackTraceType)(__in ULONG, __in ULONG, __out PVOID*, __out_opt PULONG);
    CaptureStackBackTraceType func = (CaptureStackBackTraceType)(GetProcAddress(LoadLibrary((LPCWSTR)L"ntdll.dll"), "RtlCaptureStackBackTrace"));

    fprintf(stderr, "func=%p\n", func);
    fflush(stderr);

    if(func == nullptr)
        return; 

    int nf =  (func)(1, 62, bt, &dwHashCode);
    fprintf(stderr, "callstack frames %d found\n", nf);
    fflush(stderr);

    HANDLE process;
    process = GetCurrentProcess();
    
    ::SymSetOptions( SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_UNDNAME );
    if (!::SymInitialize( process, "http://msdl.microsoft.com/download/symbols", TRUE))
    {
            DWORD error = GetLastError();
            fprintf(stderr, "SymInitialize returned error : %d\n", error);
            fflush(stderr);
    }

    //fprintf(stderr, "process=%ld\n", process);
    //fflush(stderr);
    char buffer[g_bufsize];
    //PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
    //pSymbol->MaxNameLen   = MAX_SYM_NAME;
    //pSymbol->SizeOfStruct = sizeof( SYMBOL_INFO );
    PIMAGEHLP_SYMBOL64   pSymbol = (PIMAGEHLP_SYMBOL64)buffer;
    pSymbol->MaxNameLength   = MAX_SYM_NAME;
    pSymbol->SizeOfStruct = sizeof( IMAGEHLP_SYMBOL64 );

    DWORD64  dwDisplacement = 0;
    

    size_t cur_ptr = 0;
    if (buf) buf[0] = '\0';
    for (int i=0; i < nf; i++)
    {

           //SymGetSymFromAddr64( process, ( ULONG64 )stack.AddrPC.Offset, &displacement, &symbol );
        
        //if (SymFromAddr( process, (DWORD64) (bt[i]), &dwDisplacement, pSymbol))
        if (SymGetSymFromAddr64( process, (DWORD64) (bt[i]), &dwDisplacement, pSymbol))
        {

            UnDecorateSymbolName( pSymbol->Name, ( PSTR )pSymbol->Name, MAX_SYM_NAME, UNDNAME_COMPLETE );
            fprintf(stderr, "call stack %d: %s\n",i, pSymbol->Name );
            fflush(stderr);

            if (buf)
            {
                sprintf(buf+cur_ptr, "%s\n", pSymbol->Name);
                cur_ptr += strlen(pSymbol->Name) + 2;
            }
   
        }
        else
        {
            // SymFromAddr failed
            DWORD error = GetLastError();
            fprintf(stderr, "SymFromAddr returned error for callstack %d: %d\n", error, i);
            fflush(stderr);

        }
    }
    ::SymCleanup( process );
}

void WalkCallstack( char* buf )
{
    BOOL                result;
    HANDLE              process;
    HANDLE              thread;
    CONTEXT             context;
    STACKFRAME64        stack;
    ULONG               frame;
    IMAGEHLP_SYMBOL64   symbol;
    DWORD64             displacement;
    char name[ 256 ];

    RtlCaptureContext( &context );
    memset( &stack, 0, sizeof( STACKFRAME64 ) );

    process                = GetCurrentProcess();
    thread                 = GetCurrentThread();
    displacement           = 0;
    //stack.AddrPC.Offset    = context.Eip;
    stack.AddrPC.Mode      = AddrModeFlat;
    //stack.AddrStack.Offset = context.Esp;
    stack.AddrStack.Mode   = AddrModeFlat;
    //stack.AddrFrame.Offset = context.Ebp;
    stack.AddrFrame.Mode   = AddrModeFlat;

    ::SymSetOptions( SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_UNDNAME );
    if (!::SymInitialize( process, "http://msdl.microsoft.com/download/symbols", TRUE))
    {
            DWORD error = GetLastError();
            fprintf(stderr, "WalkCallstack SymInitialize returned error : %d\n", error);
            fflush(stderr);
    }

    for( frame = 0; ; frame++ )
    {
        result = StackWalk64
        (
            IMAGE_FILE_MACHINE_I386,
            process,
            thread,
            &stack,
            &context,
            nullptr,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            nullptr
        );

        symbol.SizeOfStruct  = sizeof( IMAGEHLP_SYMBOL64 );
        symbol.MaxNameLength = 255;

        SymGetSymFromAddr64( process, ( ULONG64 )stack.AddrPC.Offset, &displacement, &symbol );
        UnDecorateSymbolName( symbol.Name, ( PSTR )name, 256, UNDNAME_COMPLETE );

        fprintf
        (
            stderr,
            "Frame %lu:\n"
            "    Symbol name:    %s\n"
            "    PC address:     0x%08LX\n"
            "    Stack address:  0x%08LX\n"
            "    Frame address:  0x%08LX\n"
            "\n",
            frame,
            symbol.Name,
            ( ULONG64 )stack.AddrPC.Offset,
            ( ULONG64 )stack.AddrStack.Offset,
            ( ULONG64 )stack.AddrFrame.Offset
        );
        fflush(stderr);

        if( !result )
        {
            break;
        }
    }
    ::SymCleanup( process );
}

#else

void CaptureCallstack(char* buf)
{
}

void DumpCallstack(char* buf)
{
}

#endif



