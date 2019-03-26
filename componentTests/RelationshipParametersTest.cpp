/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"
#include "RelationshipParameters.h"

using namespace std; 
using namespace Kernel; 

SUITE(RelationshipParametersTest)
{
    TEST(TestGoodParameters)
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/RelationshipParametersTest/TestGoodParameters.json" ) );

        RelationshipParameters parameters( RelationshipType::TRANSITORY );
        parameters.Configure( p_config.get() );

        CHECK_EQUAL( parameters.GetType(),                         RelationshipType::TRANSITORY );
        CHECK_EQUAL( parameters.GetCoitalActRate(),                0.33f );
        CHECK_EQUAL( parameters.GetDurationWeibullHeterogeneity(), 0.833333333f );
        CHECK_EQUAL( parameters.GetDurationWeibullScale(),         0.956774771214f );
        CHECK_EQUAL( parameters.GetMigrationActions().size(),      3 );
        CHECK_EQUAL( parameters.GetMigrationActions()[0],          RelationshipMigrationAction::MIGRATE   );
        CHECK_EQUAL( parameters.GetMigrationActions()[1],          RelationshipMigrationAction::TERMINATE );
        CHECK_EQUAL( parameters.GetMigrationActions()[2],          RelationshipMigrationAction::PAUSE     );
        CHECK_EQUAL( parameters.GetMigrationActionsCDF().size(),   3 );
        CHECK_EQUAL( parameters.GetMigrationActionsCDF()[0],       0.3f );
        CHECK_EQUAL( parameters.GetMigrationActionsCDF()[1],       0.8f );
        CHECK_EQUAL( parameters.GetMigrationActionsCDF()[2],       1.0f );
    }

    void TestHelper_Exception( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        try
        {

            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

            RelationshipParameters parameters( RelationshipType::TRANSITORY );

            parameters.Configure( p_config.get() );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug(msg);
                CHECK_LN( msg.find( rExpMsg ) != string::npos, lineNumber );
            }
        }
    }

    TEST(TestMigrationActionsNoElements)
    {
        TestHelper_Exception( __LINE__, "testdata/RelationshipParametersTest/TestMigrationActionsNoElements.json",
            "There must be at least one Migration Action defined." );
    }

    TEST(TestMigrationActionsNotSameNumberOfElements)
    {
        TestHelper_Exception( __LINE__, "testdata/RelationshipParametersTest/TestMigrationActionsNotSameNumberOfElements.json",
            "Variable or parameter 'Migration_Actions.size()' with value 1 is incompatible with variable or parameter 'Migration_Actions_Distribution.size()' with value 3. 'Migration_Actions' and 'Migration_Actions_Distribution' must have the same number of elements." );
    }

    TEST(TestMigrationActionsDuplicate)
    {
        TestHelper_Exception( __LINE__, "testdata/RelationshipParametersTest/TestMigrationActionsDuplicate.json",
            "Duplicate Migration_Action found = 'MIGRATE'.  There can be only one of each MigrationAction type." );
    }

    TEST(TestMigrationActionsInvalid)
    {
        TestHelper_Exception( __LINE__, "testdata/RelationshipParametersTest/TestMigrationActionsInvalid.json",
            "'ZZZ' is an unknown Migration_Action.  Possible actions are: 'PAUSE', 'MIGRATE', 'TERMINATE'" );
    }

    TEST(TestMigrationActionsDistributionGreaterThanOne)
    {
        TestHelper_Exception( __LINE__, "testdata/RelationshipParametersTest/TestMigrationActionsDistributionGreaterThanOne.json",
            "Configuration variable Migration_Actions_Distribution with value 5.5 out of range: greater than 1." );
    }

    TEST(TestMigrationActionsDistributionLessThanZero)
    {
        TestHelper_Exception( __LINE__, "testdata/RelationshipParametersTest/TestMigrationActionsDistributionLessThanZero.json",
            "Configuration variable Migration_Actions_Distribution with value -0.2 out of range: less than 0." );
    }

    TEST(TestMigrationActionsDistributionNotEqualToOne)
    {
        TestHelper_Exception( __LINE__, "testdata/RelationshipParametersTest/TestMigrationActionsDistributionNotEqualToOne.json",
            "The Migration_Actions_Distribution values do not add up to 1.0." );
    }
}