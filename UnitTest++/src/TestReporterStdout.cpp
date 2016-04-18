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

    void Print( const std::wstring& rMsg )
    {
#ifdef WIN32
        OutputDebugStringW( rMsg.c_str() );
#endif
        std::string msg( rMsg.begin(), rMsg.end() );
        printf("%s",msg.c_str());
    }

    void TestReporterStdout::ReportSummary(int const totalTestCount, int const failedTestCount,
                                           int const failureCount, float secondsElapsed)
    {
        std::wostringstream msg;
        msg << "\n-----------------------------------------------------------------------\n" ;

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
