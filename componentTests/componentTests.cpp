
#include "stdafx.h"
#include "UnitTest++.h"

using namespace std;

int main(int argc, char* argv[])
{
    if( argc == 2 )
    {
        // argv[1] is the name of a suite that you would like to run like AssortivityTest.
        // It runs all of the tests in that suite.
        return UnitTest::RunSuite( argv[1] );
    }
    else
    {
        // Run all of the test suites
        return UnitTest::RunAllTests();
    }
}

void PrintDebug( const std::string& rMessage )
{
#ifdef WIN32
    std::wostringstream msg ;
    msg << rMessage.c_str() ;
    OutputDebugStringW( msg.str().c_str() );
#else
    printf( "%s\n", rMessage.c_str() );
#endif
}
