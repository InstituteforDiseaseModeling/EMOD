/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#ifdef WIN32
#include <stdafx.h>
#endif
#include <iostream>

#pragma once

#define release_assert(x) ( (x) ? (void)0 : onAssert__(__FILE__, __LINE__, #x) )

inline void onAssert__( const char * filename, int lineNum, const char * variable )
{
    // Only permissible use of std::cerr
    std::cout << "Assertion failure, (" << variable << "), is false in file " << filename << " at line " << lineNum << std::endl;
    abort();
}

