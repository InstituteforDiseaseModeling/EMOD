/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include <memory> // unique_ptr


#include "FileSystem.h"
#include "SimulationConfig.h"
#include "JsonObjectDemog.h"
#include "IdmMpi.h"

#include "LarvalHabitatMultiplier.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"

using namespace Kernel;
using namespace std;



SUITE(LarvalHabitatMultiplierTest)
{
    struct LhmFixture
    {
        SimulationConfig* m_pSimulationConfig ;

        LhmFixture()
        : m_pSimulationConfig( new SimulationConfig() )
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;
            JsonConfigurable::_track_missing = false;
            
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            m_pSimulationConfig->vector_params->vector_species_names.insert( "arabiensis" );
            m_pSimulationConfig->vector_params->vector_species_names.insert( "funestus" );
            m_pSimulationConfig->vector_params->vector_species_names.insert( "gambiae" );

            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/LarvalHabitatMultiplierTest/config.json" ) );
            unique_ptr<Configuration> p_parameters( Environment::CopyFromElement( (*p_config)[ "parameters" ] ) );

            try
            {
                m_pSimulationConfig->vector_params->vspMap[ "arabiensis" ] = VectorSpeciesParameters::CreateVectorSpeciesParameters( p_parameters.get(), "arabiensis" );
                m_pSimulationConfig->vector_params->vspMap[ "funestus"   ] = VectorSpeciesParameters::CreateVectorSpeciesParameters( p_parameters.get(), "funestus"   );
                m_pSimulationConfig->vector_params->vspMap[ "gambiae"    ] = VectorSpeciesParameters::CreateVectorSpeciesParameters( p_parameters.get(), "gambiae"    );
            }
            catch( DetailedException& re )
            {
                PrintDebug( re.GetMsg() );
                throw re;
            }
        }

        ~LhmFixture()
        {
            delete m_pSimulationConfig;
            Environment::Finalize();
            JsonConfigurable::_useDefaults = true;
            JsonConfigurable::_track_missing = true;
        }
    };

    void CheckOneValue( LarvalHabitatMultiplier& lhm )
    {
    }

    void ConfigFromJson(JsonObjectDemog &json, LarvalHabitatMultiplier &lhm)
    {
        std::istringstream config_string(json.ToString());
        Configuration* config = Configuration::Load(config_string, std::string(""));
        lhm.Configure(config);
    }

    TEST_FIXTURE(LhmFixture, TestReadOneValueA)
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestReadOneValue.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        ConfigFromJson(json, lhm);

        for( int i = 0 ; i < VectorHabitatType::pairs::count() ; ++i )
        {
            VectorHabitatType::Enum vht = VectorHabitatType::Enum( VectorHabitatType::pairs::get_values()[i] );
            if( vht == VectorHabitatType::ALL_HABITATS )
            {
                // Only ALL_HABITATS gets the value in the file
                CHECK_CLOSE( 1.23, lhm.GetMultiplier( VectorHabitatType::ALL_HABITATS,   "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 1.23, lhm.GetMultiplier( VectorHabitatType::ALL_HABITATS,   "funestus"   ), 0.0001 );
                CHECK_CLOSE( 1.23, lhm.GetMultiplier( VectorHabitatType::ALL_HABITATS,   "gambiae"    ), 0.0001 );
            }
            else
            {
                // The other types just get the default value
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "funestus"   ), 0.0001 );
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "gambiae"    ), 0.0001 );
            }
        }
    }

    void CheckReadHabitatValue( LarvalHabitatMultiplier& lhm )
    {
        for( int i = 0 ; i < VectorHabitatType::pairs::count() ; ++i )
        {
            VectorHabitatType::Enum vht = VectorHabitatType::Enum( VectorHabitatType::pairs::get_values()[i] );
            if( vht == VectorHabitatType::BRACKISH_SWAMP )
            {
                CHECK_CLOSE( 2.0, lhm.GetMultiplier( vht, "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 2.0, lhm.GetMultiplier( vht, "funestus"   ), 0.0001 );
                CHECK_CLOSE( 2.0, lhm.GetMultiplier( vht, "gambiae"    ), 0.0001 );
            }
            else if( vht == VectorHabitatType::HUMAN_POPULATION )
            {
                CHECK_CLOSE( 3.0, lhm.GetMultiplier( vht, "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 3.0, lhm.GetMultiplier( vht, "funestus"   ), 0.0001 );
                CHECK_CLOSE( 3.0, lhm.GetMultiplier( vht, "gambiae"    ), 0.0001 );
            }
            else
            {
                // The other types just get the default value
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "funestus"   ), 0.0001 );
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "gambiae"    ), 0.0001 );
            }
        }
    }

    TEST_FIXTURE(LhmFixture, TestReadHabitatValueA)
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestReadHabitatValue.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        ConfigFromJson(json, lhm);

        CheckReadHabitatValue( lhm );
    }

    TEST_FIXTURE(LhmFixture, TestReadHabitatValueB)
    {
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/LarvalHabitatMultiplierTest/TestReadHabitatValue.json" ) );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        lhm.Configure( p_config.get() );

        CheckReadHabitatValue( lhm );
    }


    void CheckReadHabitatSpeciesValue( LarvalHabitatMultiplier& lhm )
    {
        for( int i = 0 ; i < VectorHabitatType::pairs::count() ; ++i )
        {
            VectorHabitatType::Enum vht = VectorHabitatType::Enum( VectorHabitatType::pairs::get_values()[i] );
            if( vht == VectorHabitatType::BRACKISH_SWAMP )
            {
                CHECK_CLOSE( 2.0, lhm.GetMultiplier( vht, "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 3.0, lhm.GetMultiplier( vht, "funestus"   ), 0.0001 );
                CHECK_CLOSE( 1.0, lhm.GetMultiplier( vht, "gambiae"    ), 0.0001 );
            }
            else if( vht == VectorHabitatType::TEMPORARY_RAINFALL )
            {
                CHECK_CLOSE( 1.0, lhm.GetMultiplier( vht, "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 1.0, lhm.GetMultiplier( vht, "funestus"   ), 0.0001 );
                CHECK_CLOSE( 7.0, lhm.GetMultiplier( vht, "gambiae"    ), 0.0001 );
            }
            else
            {
                // The other types just get the default value
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "funestus"   ), 0.0001 );
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "gambiae"    ), 0.0001 );
            }
        }
    }


    TEST_FIXTURE(LhmFixture, TestReadHabitatSpeciesValueA)
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestReadHabitatSpeciesValue.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        ConfigFromJson(json, lhm);

        CheckReadHabitatSpeciesValue( lhm );
    }

    TEST_FIXTURE(LhmFixture, TestReadHabitatSpeciesValueB)
    {
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/LarvalHabitatMultiplierTest/TestReadHabitatSpeciesValue.json" ) );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        lhm.Configure( p_config.get() );

        CheckReadHabitatSpeciesValue( lhm );
    }



    void CheckReadMix( LarvalHabitatMultiplier& lhm )
    {
        for( int i = 0 ; i < VectorHabitatType::pairs::count() ; ++i )
        {
            VectorHabitatType::Enum vht = VectorHabitatType::Enum( VectorHabitatType::pairs::get_values()[i] );
            if( vht == VectorHabitatType::HUMAN_POPULATION )
            {
                // set by habitat type
                CHECK_CLOSE( 5.0, lhm.GetMultiplier( vht, "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 5.0, lhm.GetMultiplier( vht, "funestus"   ), 0.0001 );
                CHECK_CLOSE( 5.0, lhm.GetMultiplier( vht, "gambiae"    ), 0.0001 );
            }
            else if( vht == VectorHabitatType::TEMPORARY_RAINFALL )
            {
                // set by species
                CHECK_CLOSE( 6.0, lhm.GetMultiplier( vht, "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 1.0, lhm.GetMultiplier( vht, "funestus"   ), 0.0001 );
                CHECK_CLOSE( 7.0, lhm.GetMultiplier( vht, "gambiae"    ), 0.0001 );
            }
            else
            {
                // The other types just get the default value
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "arabiensis" ), 0.0001 );
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "funestus"   ), 0.0001 );
                CHECK_CLOSE( 1.00, lhm.GetMultiplier( vht, "gambiae"    ), 0.0001 );
            }
        }
    }

    TEST_FIXTURE(LhmFixture, TestReadMixA)
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestReadMix.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        ConfigFromJson(json, lhm);

        CheckReadMix( lhm );
    }

    TEST_FIXTURE(LhmFixture, TestReadMixB)
    {
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/LarvalHabitatMultiplierTest/TestReadMix.json" ) );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        lhm.Configure( p_config.get() );

        CheckReadMix( lhm );
    }

    TEST_FIXTURE(LhmFixture, TestInvalidValue)
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestInvalidValue.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        try
        {
            ConfigFromJson(json, lhm);
            CHECK( false );
        }
        catch( DetailedException& /*re*/ )
        {
            //PrintDebug( re.GetMsg() );
            CHECK( true );
        }
    }

    TEST_FIXTURE( LhmFixture, TestInvalidHabitat )
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestInvalidHabitat.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        try
        {
            ConfigFromJson(json, lhm);
            CHECK( false );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( true );
        }
    }

    TEST_FIXTURE(LhmFixture, TestOverspecification)
    {
        JsonObjectDemog json;
        json.ParseFile("testdata/LarvalHabitatMultiplierTest/TestOverspecification.json");

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        try
        {
            ConfigFromJson(json, lhm);
            CHECK(false);
        }
        catch (DetailedException& re)
        {
            PrintDebug(re.GetMsg());
            CHECK(true);
        }
    }
}
