#include "TestReporterStdout.h"
#include <cstdio>
#include <string>
#include <sstream>

#include "TestDetails.h"


#ifdef WIN32
#include "windows.h"
#endif

namespace UnitTest {

    void TestReporterStdout::ReportFailure(TestDetails const& details, char const* failure)
    {
#if defined(__APPLE__) || defined(__GNUG__)
        char const* const errorFormat = "%s:%d: ERROR: Failure in %s: %s\n";
#else
        char const* const errorFormat = "%s(%d): ERROR: Failure in %s: %s\n";
#endif

        printf(errorFormat, details.filename, details.lineNumber, details.testName, failure);

        std::stringstream msg;
        msg << details.filename << "(" << details.lineNumber << "): ERROR: Failure in " << details.testName << ": " << failure << "\n" ;
        failure_list.push_back( msg.str() );
    }

    void TestReporterStdout::ReportTestStart(TestDetails const& /*test*/)
    {
    }

    void TestReporterStdout::ReportTestFinish(TestDetails const& /*test*/, float)
    {
    }

    void Print( const std::string& rMsg )
    {
        fprintf( stdout, "%s", rMsg.c_str() );
        fflush( stdout );

        fprintf( stderr, "%s", rMsg.c_str() );
        fflush( stderr );

#ifdef WIN32
        std::wstring wmsg( rMsg.begin(), rMsg.end() );
        OutputDebugStringW( wmsg.c_str() );
#endif
    }

    void TestReporterStdout::ReportSummary(int const totalTestCount, int const failedTestCount,
                                           int const failureCount, float secondsElapsed)
    {
        std::stringstream msg;

        msg << "\n-----------------------------------------------------------------------\n" ;
        msg << "componentTests\n";

        for( int i = 0 ; i < failure_list.size() ; i++ )
        {
            msg << failure_list[i] ;
        }

        if (failureCount > 0)
        {
            msg << "FAILURE: " << failedTestCount << " out of " << totalTestCount << " tests failed (" << failureCount << " failures.)\n" ;
        }
        else
        {
            msg << "SUCCESS: " << totalTestCount << " tests passed.\n" ;
        }

        msg << "Test time: " << secondsElapsed << " seconds.\n" ;
        msg << "-----------------------------------------------------------------------\n\n" ;

        Print( msg.str() );
    }
}
