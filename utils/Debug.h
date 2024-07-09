
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
    std::cerr << "Assertion failure, (" << variable << "), is false in file " << filename << " at line " << lineNum << std::endl;
    abort();
}

