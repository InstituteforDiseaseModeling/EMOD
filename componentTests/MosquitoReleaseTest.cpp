
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"


#include "MosquitoRelease.h"
#include "SimulationConfig.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "VectorTraitModifiers.h"
#include "VectorGene.h"

using namespace Kernel;

SUITE( MosquitoReleaseTest )
{
    struct MosquitoReleaseFixture
    {
        SimulationConfig*     m_pSimulationConfig;
        INodeContextFake      m_NC;
        INodeEventContextFake m_NEC;

        MosquitoReleaseFixture()
            : m_pSimulationConfig( new SimulationConfig() )
            , m_NC()
            , m_NEC()
        {
            m_pSimulationConfig->sim_type = SimType::VECTOR_SIM;

            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;
            JsonConfigurable::_track_missing = false;

            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            m_NEC.SetContextTo( &m_NC );
        }

        ~MosquitoReleaseFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = true;
            JsonConfigurable::_track_missing = true;

            delete m_pSimulationConfig;
            Environment::Finalize();
        }

        void InitializeConfig( int lineNumber, const std::string& rFilename )
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

            try
            {
                JsonConfigurable::_useDefaults = true;
                JsonConfigurable::_track_missing = true;

                m_pSimulationConfig->Configure( p_config.get() );

                m_pSimulationConfig->vector_params->vector_species.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params");
                m_pSimulationConfig->vector_params->vector_species.CheckConfiguration();

                JsonConfigurable::ClearMissingParameters();
                JsonConfigurable::_useDefaults = false;
                JsonConfigurable::_track_missing = false;
            }
            catch( DetailedException& re )
            {
                PrintDebug( re.GetMsg() );
                CHECK_LN( false, lineNumber );
            }
        }

        void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
        {
            InitializeConfig( lineNumber, "testdata/MosquitoReleaseTest/config.json" );
            try
            {

                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );
                JsonConfigurable::_useDefaults = true;
                MosquitoRelease mr;
                mr.Configure( p_config.get() );

                CHECK_LN( false, lineNumber ); // should not get here
            }
            catch( DetailedException& re )
            {
                std::string msg = re.GetMsg();
                if( msg.find( rExpMsg ) == string::npos )
                {
                    PrintDebug( rExpMsg + "\n" );
                    PrintDebug( msg + "\n" );
                    CHECK_LN( false, lineNumber );
                }
            }
        }
    };

    TEST_FIXTURE( MosquitoReleaseFixture, TestConfigure )
    {
        InitializeConfig( __LINE__, "testdata/MosquitoReleaseTest/config.json" );

        CHECK( m_pSimulationConfig->vector_params->vector_species.Size() > 0 );
        VectorSpeciesParameters* p_vsp = m_pSimulationConfig->vector_params->vector_species[0];
        CHECK_EQUAL( 4, p_vsp->genes.Size() );
        CHECK_EQUAL( 1, p_vsp->trait_modifiers.Size() );

        MosquitoRelease mr;
        try
        {

            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/MosquitoReleaseTest/TestConfigure.json" ) );
            JsonConfigurable::_useDefaults = true;
            mr.Configure( p_config.get() );

            CHECK( true );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        mr.Distribute( &m_NEC, nullptr );
        mr.Update( 1.0 );

        CHECK( mr.Expired() );

        CHECK_EQUAL( "arabiensis", m_NEC.GetMosquitoReleasedSpecies() );
        CHECK_EQUAL( false,        m_NEC.GetMosquitoReleasedIsRatio() );
        CHECK_EQUAL( 77,           m_NEC.GetMosquitoReleasedNumber() );
    }

    TEST_FIXTURE(MosquitoReleaseFixture, TestConfigureMate)
    {
        InitializeConfig(__LINE__, "testdata/MosquitoReleaseTest/config.json");

        CHECK(m_pSimulationConfig->vector_params->vector_species.Size() > 0);
        VectorSpeciesParameters* p_vsp = m_pSimulationConfig->vector_params->vector_species[0];
        CHECK_EQUAL(4, p_vsp->genes.Size());
        CHECK_EQUAL(1, p_vsp->trait_modifiers.Size());

        MosquitoRelease mr;
        try
        {
            unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/MosquitoReleaseTest/TestConfigureMate.json"));

            mr.Configure(p_config.get());

            CHECK(true);
        }
        catch (DetailedException& re)
        {
            PrintDebug(re.GetMsg());
            CHECK(false);
        }

        mr.Distribute(&m_NEC, nullptr);
        mr.Update(1.0);

        CHECK(mr.Expired());

        CHECK_EQUAL("arabiensis", m_NEC.GetMosquitoReleasedSpecies());
        CHECK_EQUAL(false,        m_NEC.GetMosquitoReleasedIsRatio());
        CHECK_EQUAL(77,           m_NEC.GetMosquitoReleasedNumber());
    }

    TEST_FIXTURE(MosquitoReleaseFixture, TestConfigureEmptyMate)
    {
        InitializeConfig(__LINE__, "testdata/MosquitoReleaseTest/config.json");

        CHECK(m_pSimulationConfig->vector_params->vector_species.Size() > 0);
        VectorSpeciesParameters* p_vsp = m_pSimulationConfig->vector_params->vector_species[0];
        CHECK_EQUAL(4, p_vsp->genes.Size());
        CHECK_EQUAL(1, p_vsp->trait_modifiers.Size());

        MosquitoRelease mr;
        try
        {
            unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/MosquitoReleaseTest/TestConfigureEmptyMate.json"));

            mr.Configure(p_config.get());

            CHECK(true);
        }
        catch (DetailedException& re)
        {
            PrintDebug(re.GetMsg());
            CHECK(false);
        }

        mr.Distribute(&m_NEC, nullptr);
        mr.Update(1.0);

        CHECK(mr.Expired());

        CHECK_EQUAL("arabiensis", m_NEC.GetMosquitoReleasedSpecies());
        CHECK_EQUAL(false,        m_NEC.GetMosquitoReleasedIsRatio());
        CHECK_EQUAL(77,           m_NEC.GetMosquitoReleasedNumber());
    }

    TEST_FIXTURE( MosquitoReleaseFixture, TestConfigureFraction )
    {
        InitializeConfig( __LINE__, "testdata/MosquitoReleaseTest/config.json" );

        CHECK( m_pSimulationConfig->vector_params->vector_species.Size() > 0 );
        VectorSpeciesParameters* p_vsp = m_pSimulationConfig->vector_params->vector_species[0];
        CHECK_EQUAL( 4, p_vsp->genes.Size() );
        CHECK_EQUAL( 1, p_vsp->trait_modifiers.Size() );

        MosquitoRelease mr;
        try
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/MosquitoReleaseTest/TestConfigureFraction.json" ) );
            JsonConfigurable::_useDefaults = true;
            mr.Configure( p_config.get() );

            CHECK( true );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        mr.Distribute( &m_NEC, nullptr );
        mr.Update( 1.0 );

        CHECK( mr.Expired() );

        CHECK_EQUAL( "funestus", m_NEC.GetMosquitoReleasedSpecies() );
        CHECK_EQUAL( true,       m_NEC.GetMosquitoReleasedIsRatio() );
        CHECK_EQUAL( 0.5,        m_NEC.GetMosquitoReleasedRatio() );
    }


    TEST_FIXTURE( MosquitoReleaseFixture, TestConfigureRatio )
    {
        InitializeConfig( __LINE__, "testdata/MosquitoReleaseTest/config.json" );

        CHECK( m_pSimulationConfig->vector_params->vector_species.Size() > 0 );
        VectorSpeciesParameters* p_vsp = m_pSimulationConfig->vector_params->vector_species[0];
        CHECK_EQUAL( 4, p_vsp->genes.Size() );
        CHECK_EQUAL( 1, p_vsp->trait_modifiers.Size() );

        MosquitoRelease mr;
        try
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/MosquitoReleaseTest/TestConfigureRatio.json" ) );
            JsonConfigurable::_useDefaults = true;
            mr.Configure( p_config.get() );

            CHECK( true );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        mr.Distribute( &m_NEC, nullptr );
        mr.Update( 1.0 );

        CHECK( mr.Expired() );

        CHECK_EQUAL( "funestus", m_NEC.GetMosquitoReleasedSpecies() );
        CHECK_EQUAL( true,       m_NEC.GetMosquitoReleasedIsRatio() );
        CHECK_EQUAL( 45.5,       m_NEC.GetMosquitoReleasedRatio() );
    }


    TEST_FIXTURE( MosquitoReleaseFixture, TestInvalidNumAllelePairs )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/MosquitoReleaseTest/TestInvalidNumAllelePairs.json",
                                       "The parameter 'Released_Genome' does not have an allele pair for each gene/loci.\nThe parameter has 3 pairs and there are 3 defined plus the gender gene." );
    }

    TEST_FIXTURE( MosquitoReleaseFixture, TestInvalidNumAlleleInPair )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/MosquitoReleaseTest/TestInvalidNumAlleleInPair.json",
                                       "The parameter 'Released_Genome' has the allele pair #3 with\n3 elements instead of two." );
    }

    TEST_FIXTURE(MosquitoReleaseFixture, TestInvalidNumAllelePairsMate)
    {
        TestHelper_ConfigureException(__LINE__, "testdata/MosquitoReleaseTest/TestInvalidNumAllelePairs.json",
            "The parameter 'Released_Genome' does not have an allele pair for each gene/loci.\nThe parameter has 3 pairs and there are 3 defined plus the gender gene.");
    }

    TEST_FIXTURE(MosquitoReleaseFixture, TestInvalidNumAlleleInPairMate)
    {
        TestHelper_ConfigureException(__LINE__, "testdata/MosquitoReleaseTest/TestInvalidNumAlleleInPair.json",
            "The parameter 'Released_Genome' has the allele pair #3 with\n3 elements instead of two.");
    }

    TEST_FIXTURE( MosquitoReleaseFixture, TestInvalidMaleVectorInfection )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/MosquitoReleaseTest/TestInvalidMaleVectorInfection.json",
                                       "'Released_Infectious' > 0 and cannot be used with male vectors." );
    }

    TEST_FIXTURE(MosquitoReleaseFixture, TestInvalidFemaleMate)
    {
        TestHelper_ConfigureException(__LINE__, "testdata/MosquitoReleaseTest/TestInvalidFemaleMate.json",
            "When 'Released_Mate_Genome' is defined, it must be male and 'Released_Genome' must be female.");
    }

    TEST_FIXTURE(MosquitoReleaseFixture, TestInvalidMaleReleaseMate)
    {
        TestHelper_ConfigureException(__LINE__, "testdata/MosquitoReleaseTest/TestInvalidMaleReleaseMate.json",
            "When 'Released_Mate_Genome' is defined, it must be male and 'Released_Genome' must be female.");
    }
}
