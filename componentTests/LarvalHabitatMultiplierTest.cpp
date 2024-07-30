
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

            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/LarvalHabitatMultiplierTest/config.json" ) );

            try
            {
                m_pSimulationConfig->vector_params->vector_species.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params" );
                m_pSimulationConfig->vector_params->vector_species.CheckConfiguration();
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
            JsonConfigurable::ClearMissingParameters();
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

    TEST_FIXTURE( LhmFixture, TestReadMixA )
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestReadMix.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        ConfigFromJson( json, lhm );

        CheckReadMix( lhm );
    }

    TEST_FIXTURE( LhmFixture, TestReadMixB )
    {
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/LarvalHabitatMultiplierTest/TestReadMix.json" ) );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        lhm.Configure( p_config.get() );

        CheckReadMix( lhm );
    }

    TEST_FIXTURE( LhmFixture, TestInvalidValue )
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestInvalidValue.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        try
        {
            ConfigFromJson( json, lhm );
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
            ConfigFromJson( json, lhm );
            CHECK( false );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( true );
        }
    }

    TEST_FIXTURE( LhmFixture, TestHabitatNotUsedA )
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestHabitatNotUsedA.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        try
        {
            ConfigFromJson( json, lhm );
            CHECK( false );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( true );
        }
    }

    TEST_FIXTURE( LhmFixture, TestHabitatNotUsedB )
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestHabitatNotUsedB.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        try
        {
            ConfigFromJson( json, lhm );
            CHECK( false );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( true );
        }
    }

    TEST_FIXTURE( LhmFixture, TestUnknownSpecies )
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestUnknownSpecies.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        try
        {
            ConfigFromJson( json, lhm );
            CHECK( false );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( true );
        }
    }

    TEST_FIXTURE( LhmFixture, TestOverspecification )
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestOverspecification.json" );

        LarvalHabitatMultiplier lhm;
        lhm.Initialize();

        try
        {
            ConfigFromJson( json, lhm );
            CHECK( false );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( true );
        }
    }

    TEST_FIXTURE( LhmFixture, TestInvalidAllHabitatsInIntervention )
    {
        JsonObjectDemog json;
        json.ParseFile( "testdata/LarvalHabitatMultiplierTest/TestInvalidAllHabitatsInIntervention.json" );

        LarvalHabitatMultiplier lhm( true ); // used by intervention
        lhm.Initialize();

        try
        {
            ConfigFromJson( json, lhm );
            CHECK( false );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( true );
        }
    }

}
