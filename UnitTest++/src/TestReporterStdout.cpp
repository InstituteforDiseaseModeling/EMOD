#include "TestReporterStdout.h"
#include <cstdio>

#include "TestDetails.h"


#ifdef WIN32
#include "windows.h"
#include <string>
#include <sstream>
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
#ifdef WIN32
        std::wostringstream msg;
        msg << details.filename << "(" << details.lineNumber << "): ERROR: Failure in " << details.testName << ": " << failure << "\n" ;
        failure_list.push_back( msg.str() );
#endif
    }

    void TestReporterStdout::ReportTestStart(TestDetails const& /*test*/)
    {
    }

    void TestReporterStdout::ReportTestFinish(TestDetails const& /*test*/, float)
    {
    }

    void TestReporterStdout::ReportSummary(int const totalTestCount, int const failedTestCount,
                                           int const failureCount, float secondsElapsed)
    {
        using namespace std;

#ifdef WIN32
        OutputDebugStringW( wstring(L"\n").c_str() );

        for( int i = 0 ; i < failure_list.size() ; i++ )
        {
            OutputDebugStringW( failure_list[i].c_str() );
        }
#endif

        if (failureCount > 0)
        {
            printf("FAILURE: %d out of %d tests failed (%d failures).\n", failedTestCount, totalTestCount, failureCount);
#ifdef WIN32
            wostringstream msg;
            msg << "FAILURE: " << failedTestCount << " out of " << totalTestCount << " tests failed (" << failureCount << " failures.)\n" ;
            OutputDebugStringW( msg.str().c_str() );
#endif
        }
        else
        {
            printf("Success: %d tests passed.\n", totalTestCount);
#ifdef WIN32
            std::wostringstream msg;
            msg << "SUCCESS: " << totalTestCount << " tests passed.\n" ;
            OutputDebugStringW( msg.str().c_str() );
#endif
        }

        printf("Test time: %.2f seconds.\n", secondsElapsed);
#ifdef WIN32
        std::wostringstream msg;
        msg << "Test time: " << secondsElapsed << " seconds.\n\n" ;
        OutputDebugStringW( msg.str().c_str() );
#endif
    }
}
