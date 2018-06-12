/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef WIN32
    #include <windows.h>
    #if defined(_DEBUG)
        #define USE_DEBUG_NEW
        #ifdef USE_DEBUG_NEW
            #define _CRTDBG_MAP_ALLOC
            #include <stdlib.h>
            #include <crtdbg.h>
            #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
            #define _new_ DEBUG_NEW
        #else

            #define _new_ new
        #endif
    #else

        #define _new_ new
    #endif
#else  // not doing anything special on unix

    #define _new_ new
#endif

#ifdef WIN32
    #ifndef _WIN32_WINNT               // Allow use of features specific to Windows Vista or later. 
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! We want Vista so that CaptureStackBackTrace() in Exceptions.cpp is supported 
        // !!! _WIN32_WINNT_WS03  (0x0502)
        // !!! _WIN32_WINNT_VISTA (0x0600)
        // !!! _WIN32_WINNT_WIN7  (0x0601)
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        #define _WIN32_WINNT 0x0600    // Change this to the appropriate value to target other versions of Windows.
    #endif                        
#endif

#include <stdio.h>
#include <cstdlib>


#ifdef WIN32    // hope i dont need this for gcc

    #include <tchar.h>
    #include "stdint.h" // version of stdint.h for ms compilers; this file is manually included in the project because it does not ship with Visual Studio 2008
#endif


#ifndef WIN32

    #define sprintf_s sprintf

#endif

// TODO: reference additional headers here
