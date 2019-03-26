/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"

#include "FileSystem.h"
#include "InterpolatedValueMap.h"

#include <string>
#include <climits>
#include <vector>
#include <iostream>
#include <memory> // unique_ptr

using namespace Kernel;
using namespace std;


SUITE(InterpolatedValueMapTest)
{
    TEST(TestValid)
    {
        InterpolatedValueMap map ;

        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/InterpolatedValueMapTest/Valid.json" ) );

        map.ConfigureFromJsonAndKey( p_config.get(), "ValidData" );

        CHECK( map.count(   0) == 0 );
        CHECK( map.count(1000) > 0 );
        CHECK( map.count(2000) > 0 );
        CHECK( map.count(3000) > 0 );
        CHECK( map.count(4000) == 0 );

        CHECK_EQUAL( map[1000], 1 );
        CHECK_EQUAL( map[2000], 2 );
        CHECK_EQUAL( map[3000], 3 );
    }

    TEST(TestBadKeyName)
    {
        try
        {

            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/InterpolatedValueMapTest/TestBadKeyName.json" ) );

            InterpolatedValueMap map ;

            map.ConfigureFromJsonAndKey( p_config.get(), "BadKeyName" );

            CHECK( false ); // should not get here
        }
        catch( json::Exception& re )
        {
            std::string msg = re.what();
            std::cout << msg << std::endl ;
            CHECK( msg.find( "Object name not found: BadKeyName" ) != string::npos );
        }
    }

    void TestHelper_Exception( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        try
        {

            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

            InterpolatedValueMap map ;

            map.ConfigureFromJsonAndKey( p_config.get(), "MyDataMap" );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            //std::cout << msg << std::endl ;
            CHECK_LN( msg.find( rExpMsg ) != string::npos, lineNumber );
        }
    }

    TEST(TestNotSameNumberOfElementsA)
    {
        TestHelper_Exception( __LINE__, "testdata/InterpolatedValueMapTest/TestNotSameNumberOfElementsA.json",
            "MyDataMap: The number of elements in Times (=3) does not match the number of elements in Values (=2)." );
    }

    TEST(TestNotSameNumberOfElementsB)
    {
        TestHelper_Exception( __LINE__, "testdata/InterpolatedValueMapTest/TestNotSameNumberOfElementsB.json",
            "MyDataMap: The number of elements in Times (=2) does not match the number of elements in Values (=3)." );
    }

    TEST(TestNegativeTime)
    {
        TestHelper_Exception( __LINE__, "testdata/InterpolatedValueMapTest/TestNegativeTime.json",
            "Configuration variable MyDataMap:Times with value -2000 out of range: less than 0." );
    }

    TEST(TestNegativeValue)
    {
        TestHelper_Exception( __LINE__, "testdata/InterpolatedValueMapTest/TestNegativeValue.json",
            "Configuration variable MyDataMap:Values with value -2 out of range: less than 0." );
    }

    TEST(TestTimeTooLarge)
    {
        TestHelper_Exception( __LINE__, "testdata/InterpolatedValueMapTest/TestTimeTooLarge.json",
            "Configuration variable MyDataMap:Times with value 1e+06 out of range: greater than 999999." );
    }

    TEST(TestBadTimeOrder)
    {
        TestHelper_Exception( __LINE__, "testdata/InterpolatedValueMapTest/TestBadTimeOrder.json",
            "MyDataMap:Times - Element number 2 (=2000) is <= element number 1 (=3000).  'Times' must be in increasing order." );
    }
}