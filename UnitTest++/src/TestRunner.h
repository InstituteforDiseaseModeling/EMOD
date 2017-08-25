#ifndef UNITTEST_TESTRUNNER_H
#define UNITTEST_TESTRUNNER_H

#include <string>

#include "Test.h"
#include "TestList.h"
#include "CurrentTest.h"

namespace UnitTest {

class TestReporter;
class TestResults;
class Timer;

int RunAllTests();
int RunSuite( const std::string& testSuiteName );

struct True
{
    bool operator()(const Test* const) const
    {
        return true;    
    }
};

class TestRunner
{
public:
    explicit TestRunner(TestReporter& reporter);
    ~TestRunner();

    template <class Predicate>
    int RunTestsIf(TestList const& list, char const* suiteName, 
                   const Predicate& predicate, int maxTestTimeInMs) const
    {
        Test* curTest = list.GetHead();

        while (curTest != 0)
        {
            if (IsTestInSuite(curTest,suiteName) && predicate(curTest))
            {
                printf( "<<<<<<<<<<<<<<<< %s - %s - START >>>>>>>>>>>>>>>>>>>\n", curTest->m_details.suiteName, curTest->m_details.testName ); fflush( stdout );
                RunTest(m_result, curTest, maxTestTimeInMs);
                printf( "<<<<<<<<<<<<<<<< %s - %s - END   >>>>>>>>>>>>>>>>>>>\n", curTest->m_details.suiteName, curTest->m_details.testName ); fflush( stdout );
            }

            curTest = curTest->next;
        }

        return Finish();
    }    

    int RunTests( TestList const& list, const std::string& testSuiteName );

private:
    TestReporter* m_reporter;
    TestResults* m_result;
    Timer* m_timer;

    int Finish() const;
    bool IsTestInSuite(const Test* const curTest, char const* suiteName) const;
    void RunTest(TestResults* const result, Test* const curTest, int const maxTestTimeInMs) const;
};

}

#endif
