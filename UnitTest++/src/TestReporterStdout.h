#ifndef UNITTEST_TESTREPORTERSTDOUT_H
#define UNITTEST_TESTREPORTERSTDOUT_H

#include <vector>
#include <string>
#include "TestReporter.h"

namespace UnitTest {

class TestReporterStdout : public TestReporter
{
public:
    TestReporterStdout()
    : TestReporter()
    , failure_list()
    {
    };
private:
    virtual void ReportTestStart(TestDetails const& test);
    virtual void ReportFailure(TestDetails const& test, char const* failure);
    virtual void ReportTestFinish(TestDetails const& test, float secondsElapsed);
    virtual void ReportSummary(int totalTestCount, int failedTestCount, int failureCount, float secondsElapsed);

    std::vector<std::string>  failure_list ;
};

}

#endif 
