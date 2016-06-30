/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "common.h"
#include "ConcurrencyParameters.h"
#include "RANDOM.h"

using namespace std; 
using namespace Kernel; 

SUITE(ConcurrencyParametersTest)
{
    struct ConcurrencyParametersFixture
    {
        bool track_missing;
        bool use_defaults;

        ConcurrencyParametersFixture()
        {
            track_missing = JsonConfigurable::_track_missing;
            use_defaults  = JsonConfigurable::_useDefaults;
            JsonConfigurable::_track_missing = false;
            JsonConfigurable::_useDefaults = false ;

            JsonConfigurable::ClearMissingParameters();
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );

            const_cast<Environment*>(Environment::getInstance())->RNG = new PSEUDO_DES(0);
        }

        ~ConcurrencyParametersFixture()
        {
            JsonConfigurable::_track_missing = track_missing;
            JsonConfigurable::_useDefaults   = use_defaults ;
            JsonConfigurable::ClearMissingParameters();
            Environment::Finalize();
        }
    };

    ConcurrencyConfiguration* ReadConcurrency( const tPropertiesDistrib& rProperties, const char* filename )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( filename ) );

        ConcurrencyConfiguration* p_cc = new ConcurrencyConfiguration();

        unique_ptr<Configuration> p_concurrency_config( Environment::CopyFromElement( (*p_config)[ "Concurrency_Configuration" ] ) );
        p_cc->Initialize( rProperties, p_concurrency_config.get() );

        const std::string& r_con_prop_key = p_cc->GetPropertyKey();

        for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
        {
            std::string main_element_name = RelationshipType::pairs::lookup_key(irel);
            unique_ptr<Configuration> p_cp_config( Environment::CopyFromElement( (*p_config)[ main_element_name ][ "Concurrency_Parameters" ] ) );
 
            RelationshipType::Enum rel_type = (RelationshipType::Enum)irel ;

            ConcurrencyParameters* p_cp = new ConcurrencyParameters();
            p_cp->Initialize( main_element_name, r_con_prop_key, rProperties, p_cp_config.get() );
            p_cc->AddParameters( rel_type, p_cp );
        }
        return p_cc;
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestGoodParametersRisk)
    {
        std::multimap< float, std::string > risk_percent_to_value;
        risk_percent_to_value.insert( std::make_pair( 0.2f, "LOW" ) );
        risk_percent_to_value.insert( std::make_pair( 0.5f, "MED" ) );
        risk_percent_to_value.insert( std::make_pair( 0.3f, "HIGH" ) );

        std::multimap< float, std::string > accessibility_percent_to_value;
        accessibility_percent_to_value.insert( std::make_pair( 0.2f, "NO" ) );
        accessibility_percent_to_value.insert( std::make_pair( 0.8f, "YES" ) );

        tPropertiesDistrib properties_distrib;
        properties_distrib.insert( std::make_pair( "RISK", risk_percent_to_value ) );
        properties_distrib.insert( std::make_pair( "ACCESSIBILITY", accessibility_percent_to_value ) );

        tProperties individual_properties;
        individual_properties.insert( std::make_pair( "ACCESSIBILITY", "YES" ) );
        individual_properties.insert( std::make_pair( "RISK", "MED" ) );

        ConcurrencyConfiguration* tmp_p_cc = nullptr;
        try
        {
            tmp_p_cc = ReadConcurrency( properties_distrib, "testdata/ConcurrencyParametersTest/TestGoodParametersRisk.json" );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
        unique_ptr<ConcurrencyConfiguration> p_cc(tmp_p_cc);

        CHECK_EQUAL( std::string("RISK"), p_cc->GetPropertyKey() );
        CHECK_EQUAL( 0.77f,               p_cc->GetProbSuperSpreader() );
        CHECK ( p_cc->IsConcurrencyProperty( "RISK" ) );
        CHECK ( !p_cc->IsConcurrencyProperty( "ACCESSIBILITY" ) );
        CHECK_EQUAL( std::string("MED" ), std::string(p_cc->GetConcurrencyPropertyValue( &individual_properties, nullptr, nullptr )) );
        CHECK_EQUAL( std::string("HIGH"), std::string(p_cc->GetConcurrencyPropertyValue( &individual_properties, "RISK" , "HIGH" )) );

        CHECK_EQUAL( 15, p_cc->GetProbExtraRelationalBitMask( "RISK", "HIGH", Gender::FEMALE, true  ) ); 
        CHECK_EQUAL(  0, p_cc->GetProbExtraRelationalBitMask( "RISK", "HIGH", Gender::FEMALE, false ) ); 
        CHECK_EQUAL( 11, p_cc->GetProbExtraRelationalBitMask( "RISK", "HIGH", Gender::MALE,   false ) ); 
        CHECK_EQUAL(  0, p_cc->GetProbExtraRelationalBitMask( "RISK", "MED",  Gender::FEMALE, false ) ); 
        CHECK_EQUAL(  8, p_cc->GetProbExtraRelationalBitMask( "RISK", "MED",  Gender::MALE,   false ) ); 
        CHECK_EQUAL(  8, p_cc->GetProbExtraRelationalBitMask( "RISK", "LOW",  Gender::FEMALE, false ) ); 
        CHECK_EQUAL( 10, p_cc->GetProbExtraRelationalBitMask( "RISK", "LOW",  Gender::MALE,   false ) ); 

        CHECK_EQUAL( 13, p_cc->GetMaxAllowableRelationships( "RISK", "HIGH", Gender::FEMALE, RelationshipType::COMMERCIAL ) );
        CHECK_EQUAL(  3, p_cc->GetMaxAllowableRelationships( "RISK", "HIGH", Gender::MALE,   RelationshipType::MARITAL       ) );
        CHECK_EQUAL(  6, p_cc->GetMaxAllowableRelationships( "RISK", "HIGH", Gender::FEMALE, RelationshipType::INFORMAL      ) );
        CHECK_EQUAL(  8, p_cc->GetMaxAllowableRelationships( "RISK", "HIGH", Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  8, p_cc->GetMaxAllowableRelationships( "RISK", "MED",  Gender::FEMALE, RelationshipType::COMMERCIAL ) );
        CHECK_EQUAL(  1, p_cc->GetMaxAllowableRelationships( "RISK", "MED",  Gender::FEMALE, RelationshipType::MARITAL       ) );
        CHECK_EQUAL(  6, p_cc->GetMaxAllowableRelationships( "RISK", "MED",  Gender::MALE,   RelationshipType::INFORMAL      ) );
        CHECK_EQUAL(  2, p_cc->GetMaxAllowableRelationships( "RISK", "MED",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  5, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::COMMERCIAL ) );
        CHECK_EQUAL(  0, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::MARITAL       ) );
        CHECK_EQUAL(  3, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::INFORMAL      ) );
        CHECK_EQUAL(  2, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::MALE,   RelationshipType::TRANSITORY    ) );

        // value is 1.5 see that half are 1 and half are 2
        CHECK_EQUAL(  2, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  2, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  1, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  1, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  2, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  1, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  2, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  2, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  1, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
        CHECK_EQUAL(  1, p_cc->GetMaxAllowableRelationships( "RISK", "LOW",  Gender::FEMALE, RelationshipType::TRANSITORY    ) );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestGoodParametersNone)
    {
        std::multimap< float, std::string > accessibility_percent_to_value;
        accessibility_percent_to_value.insert( std::make_pair( 0.2f, "NO" ) );
        accessibility_percent_to_value.insert( std::make_pair( 0.8f, "YES" ) );

        tPropertiesDistrib properties_distrib;
        properties_distrib.insert( std::make_pair( "ACCESSIBILITY", accessibility_percent_to_value ) );

        tProperties individual_properties;
        individual_properties.insert( std::make_pair( "ACCESSIBILITY", "YES" ) );

        ConcurrencyConfiguration* tmp_p_cc = nullptr;
        try
        {
            tmp_p_cc = ReadConcurrency( properties_distrib, "testdata/ConcurrencyParametersTest/TestGoodParametersNone.json" );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
        unique_ptr<ConcurrencyConfiguration> p_cc(tmp_p_cc);

        CHECK_EQUAL( std::string("NONE"), p_cc->GetPropertyKey() );
        CHECK_EQUAL( 0.88f,               p_cc->GetProbSuperSpreader() );
        CHECK ( p_cc->IsConcurrencyProperty( "NONE" ) );
        CHECK ( !p_cc->IsConcurrencyProperty( "ACCESSIBILITY" ) );
        CHECK_EQUAL( std::string("NONE"), std::string(p_cc->GetConcurrencyPropertyValue( &individual_properties, nullptr, nullptr )) );
        CHECK_EQUAL( std::string("NONE"), std::string(p_cc->GetConcurrencyPropertyValue( &individual_properties, "ACCESSIBILITY", "YES" )) );

        CHECK_EQUAL( 15, p_cc->GetProbExtraRelationalBitMask( "NONE", "NONE", Gender::FEMALE, true  ) ); 
        CHECK_EQUAL( 10, p_cc->GetProbExtraRelationalBitMask( "NONE", "NONE", Gender::FEMALE, false ) ); 
        CHECK_EQUAL(  8, p_cc->GetProbExtraRelationalBitMask( "NONE", "NONE", Gender::MALE,   false ) ); 

        CHECK_EQUAL(  5, p_cc->GetMaxAllowableRelationships( "NONE", "NONE", Gender::FEMALE, RelationshipType::COMMERCIAL ) );
        CHECK_EQUAL(  1, p_cc->GetMaxAllowableRelationships( "NONE", "NONE", Gender::MALE,   RelationshipType::MARITAL       ) );
        CHECK_EQUAL(  3, p_cc->GetMaxAllowableRelationships( "NONE", "NONE", Gender::FEMALE, RelationshipType::INFORMAL      ) );
        CHECK_EQUAL(  2, p_cc->GetMaxAllowableRelationships( "NONE", "NONE", Gender::MALE,   RelationshipType::TRANSITORY    ) );
    }

    void TestHelper_Exception( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        std::multimap< float, std::string > risk_percent_to_value;
        risk_percent_to_value.insert( std::make_pair( 0.7f, "LOW" ) );
        risk_percent_to_value.insert( std::make_pair( 0.3f, "HIGH" ) );

        std::multimap< float, std::string > accessibility_percent_to_value;
        accessibility_percent_to_value.insert( std::make_pair( 0.2f, "NO" ) );
        accessibility_percent_to_value.insert( std::make_pair( 0.8f, "YES" ) );

        tPropertiesDistrib properties_distrib;
        properties_distrib.insert( std::make_pair( "ACCESSIBILITY", accessibility_percent_to_value ) );
        properties_distrib.insert( std::make_pair( "RISK", risk_percent_to_value ) );

        try
        {
            unique_ptr<ConcurrencyConfiguration> p_cc( ReadConcurrency( properties_distrib, rFilename.c_str() ) );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            std::size_t tmp =  msg.find( rExpMsg );
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug(msg);
                CHECK_LN( msg.find( rExpMsg ) != string::npos, lineNumber );
            }
        }
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConfigPropertyName)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConfigPropertyName.json",
            "'Individual_Property_Name' (='BAD') in 'Concurrency_Configuration' in the demograhics is not a defined Individual Property.\nValid Individual Properties are: 'ACCESSIBILITY' 'RISK'" );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConfigPropertyNameEmpty)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConfigPropertyNameEmpty.json",
            "'Individual_Property_Name' cannot be empty string." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConfigMissingPropertyValue)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConfigMissingPropertyValue.json",
            "Parameter 'LOW' not found in input file 'N/A'.\n Occured while reading 'Concurrency_Configuration' from the demographics." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConfigMissingSuperSpreader)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConfigMissingSuperSpreader.json",
            "Parameter 'Probability_Person_Is_Behavioral_Super_Spreader' not found in input file 'N/A'.\n Occured while reading 'Concurrency_Configuration' from the demographics." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConfigInvalidSuperSpreader)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConfigInvalidSuperSpreader.json",
            "Configuration variable Probability_Person_Is_Behavioral_Super_Spreader with value -0.77 out of range: less than or equal to 0. Occured while reading 'Concurrency_Configuration' from the demographics." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConfigMissingExtraRelFlag)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConfigMissingExtraRelFlag.json",
            "While trying to parse json data for param >>> Extra_Relational_Flag_Type <<< in otherwise valid json segment..." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConfigInvalidExtraRelFlag)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConfigInvalidExtraRelFlag.json",
            "Failed to find enum match for value Independent_XXX and key Extra_Relational_Flag_Type. Possible values are: Independent, Correlated, COUNT\n Occured while reading 'Concurrency_Configuration' from the demographics." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConfigRelTypeOrderInvalidValue)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConfigRelTypeOrderInvalidValue.json",
            "Constrained strings (dynamic enum) with specified value TRANSITORY_XXX invalid. Possible values are: COMMERCIAL...INFORMAL...MARITAL...TRANSITORY...\n Occured while reading 'Concurrency_Configuration' from the demographics." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConfigRelTypeOrderNotEnough)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConfigRelTypeOrderNotEnough.json",
            "Reading 'HIGH: 'Correlated_Relationship_Type_Order' has 3 types and must have 4\n Occured while reading 'Concurrency_Configuration' from the demographics." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConfigRelTypeOrderDuplicate)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConfigRelTypeOrderDuplicate.json",
            "Duplicate(='MARITAL') found in 'Correlated_Relationship_Type_Order'.  There must be one and only one of each RelationshipType.\n Occured while reading 'Concurrency_Configuration' from the demographics." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConcurrMissingPropertyValue)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConcurrMissingPropertyValue.json",
            "Parameter 'HIGH' not found in input file 'N/A'.\n Occured while reading the 'Concurrency_Parameters' in 'TRANSITORY' from the demographics." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConcurrMissingMax)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConcurrMissingMax.json",
            "Parameter 'Max_Simultaneous_Relationships_Male' not found in input file 'N/A'.\n Occured while reading the 'Concurrency_Parameters' in 'TRANSITORY' from the demographics." );
    }

    TEST_FIXTURE(ConcurrencyParametersFixture, TestBadConcurrInvalidMax)
    {
        TestHelper_Exception( __LINE__, "testdata/ConcurrencyParametersTest/TestBadConcurrInvalidMax.json",
            "Configuration variable Max_Simultaneous_Relationships_Male with value -9 out of range: less than or equal to 0. Occured while reading the 'Concurrency_Parameters' in 'TRANSITORY' from the demographics." );
    }
}