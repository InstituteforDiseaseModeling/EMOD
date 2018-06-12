/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Configure.h"
#include "UnitTest++.h"
#include <string>


using namespace Kernel;

class TestConfigure : public Kernel::JsonConfigurable
{
public:
    TestConfigure() :JsonConfigurable() {};
    QueryResult QueryInterface(iid_t iid, void** pinstance) { return (QueryResult)0; };
    int32_t AddRef(void) { return 0; };
    int32_t Release(void) { return 0; };

    //JsonConfigurable::EnforceParameterAscending is not public
    void EnforceParameterAscending(const std::vector<int> & values)
    {
        std::string dummy("dummy_string");
        JsonConfigurable::EnforceParameterAscending(dummy, values);
    }
};


SUITE(ConfigureTest)
{
    TEST(EnforceParameterAscending_ascending)
    {
        TestConfigure testConfigure;
        const std::vector<int> values = { 1,2,3 };
        testConfigure.EnforceParameterAscending(values);
    }

    TEST(EnforceParameterAscending_ascending_neg)
    {
        TestConfigure testConfigure;
        const std::vector<int> values = { -1,2,3 };
        testConfigure.EnforceParameterAscending(values);
    }

    TEST(EnforceParameterAscending_descending)
    {
        TestConfigure testConfigure;
        const std::vector<int> values = { 1,2,1 };
        CHECK_THROW(testConfigure.EnforceParameterAscending(values), InvalidInputDataException);
    }

    TEST(EnforceParameterAscending_unique)
    {
        TestConfigure testConfigure;
        std::vector<int> values = { 1,3,3 };
        CHECK_THROW(testConfigure.EnforceParameterAscending(values), InvalidInputDataException);
    }

    TEST(EnforceParameterAscending_check_exception_msg)
    {
        try
        {
            throw InvalidInputDataException("test_filename", -1,"test_function_name", "");
        }
        catch (const InvalidInputDataException &e)
        {
            std::string tmp(e.what());
            CHECK_EQUAL(std::string("\nException in test_filename at -1 in test_function_name.\n"), e.what());
        }
    }
}