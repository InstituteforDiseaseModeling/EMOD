
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
#include "VectorSurveillanceEventCoordinator.h"
#include "SimulationEventContext.h"
#include "InterventionFactory.h"
#include "ReportStatsByIP.h"
#include "SimulationConfig.h"
#include "VectorSpeciesParameters.h"
#include "VectorParameters.h"
#include "VectorPopulation.h"



using namespace Kernel;

SUITE( VectorCounterTest )
{
    struct VectorCounterFixture
    {
        SimulationConfig*     m_pSimulationConfig;
        INodeContextFake      m_NC;
        INodeEventContextFake m_NEC;

        VectorCounterFixture()
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

        ~VectorCounterFixture()
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
            InitializeConfig( lineNumber, "testdata/VectorCounterTest/config.json" );
            try
            {
                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

                VectorCounter vc;
                vc.Configure( p_config.get() );

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

    TEST_FIXTURE(VectorCounterFixture, TestConfigure )
    {
        InitializeConfig( __LINE__, "testdata/VectorCounterTest/config.json" );

        CHECK( m_pSimulationConfig->vector_params->vector_species.Size() > 0 );
        VectorSpeciesParameters* p_vsp = m_pSimulationConfig->vector_params->vector_species[0];
        CHECK_EQUAL( 4, p_vsp->genes.Size() );
        CHECK_EQUAL( 1, p_vsp->trait_modifiers.Size() );

        VectorCounter vc;
        CHECK_EQUAL(VectorCounterType::ALLELE_FREQ, vc.GetCounterType());
        CHECK_EQUAL("", vc.GetSpecies().parameter_name);
        CHECK_EQUAL(30, vc.GetUpdatePeriod());
        CHECK_EQUAL(VectorGender::VECTOR_FEMALE, vc.GetGender());

        try
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorCounterTest/TestConfigure.json" ) );

            vc.Configure( p_config.get() );

            CHECK( true );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        // create a nodecontextfake, create a vector population, add vector population to nodecontextfake, list of nodeeventcontexts (SetContextTo)
        std::vector<INodeEventContext*> cached_Nodes;
        INodeContextFake node_context;
        INodeEventContextFake node_event_context;
        node_event_context.SetContextTo(&node_context);
        IVectorPopulation* p_vp((VectorPopulation*)VectorPopulation::CreatePopulation(&node_context, 0, 10000));

        node_context.AddVectorPopulationToNode(p_vp);
        cached_Nodes.push_back(&node_event_context);

        CHECK_EQUAL(VectorCounterType::ALLELE_FREQ, vc.GetCounterType());
        CHECK_EQUAL("arabiensis", vc.GetSpecies());
        CHECK_EQUAL(15, vc.GetUpdatePeriod());
        CHECK_EQUAL(VectorGender::VECTOR_FEMALE, vc.GetGender());

        // ---------------------------------------------
        // --- Check CollectStatistics() - ALLELE_FREQ
        // ---------------------------------------------
        vc.CollectStatistics( cached_Nodes );

        CHECK_EQUAL( 1283, vc.GetNumVectorsSampled() );

        const std::vector<std::string>& r_allele_names = vc.GetNames();
        CHECK_EQUAL( 9, r_allele_names.size() );
        CHECK_EQUAL( "X",        r_allele_names[ 0 ] );
        CHECK_EQUAL( "Y",        r_allele_names[ 1 ] );
        CHECK_EQUAL( "Modified", r_allele_names[ 2 ] );
        CHECK_EQUAL( "Wild",     r_allele_names[ 3 ] );
        CHECK_EQUAL( "b0",       r_allele_names[ 4 ] );
        CHECK_EQUAL( "b1",       r_allele_names[ 5 ] );
        CHECK_EQUAL( "c0",       r_allele_names[ 6 ] );
        CHECK_EQUAL( "c1",       r_allele_names[ 7 ] );
        CHECK_EQUAL( "c2",       r_allele_names[ 8 ] );

        const std::vector<float>& r_fractions = vc.GetFractions();
        CHECK_EQUAL( 9, r_fractions.size() );
        CHECK_CLOSE( 1.0,     r_fractions[ 0 ], 0.00001 );
        CHECK_CLOSE( 0.0,     r_fractions[ 1 ], 0.00001 );
        CHECK_CLOSE( 0.0,     r_fractions[ 2 ], 0.00001 );
        CHECK_CLOSE( 1.0,     r_fractions[ 3 ], 0.00001 );
        CHECK_CLOSE( 0.29501, r_fractions[ 4 ], 0.00001 );// b0
        CHECK_CLOSE( 0.70499, r_fractions[ 5 ], 0.00001 );
        CHECK_CLOSE( 0.50117, r_fractions[ 6 ], 0.00001 ); // c0
        CHECK_CLOSE( 0.49883, r_fractions[ 7 ], 0.00001 );
        CHECK_CLOSE( 0.0,     r_fractions[ 8 ], 0.00001 );


        // ---------------------------------------------
        // --- Check CollectStatistics() - GENOME_FRACTION
        // ---------------------------------------------
        vc.SetCounterType( VectorCounterType::GENOME_FRACTION );

        vc.CollectStatistics( cached_Nodes );

        CHECK_EQUAL( 955, vc.GetNumVectorsSampled() );

        const std::vector<std::string>& r_allele_names2 = vc.GetNames();
        const std::vector<float>&       r_fractions2    = vc.GetFractions();
        CHECK_EQUAL( 54, r_allele_names2.size() );
        CHECK_EQUAL( 54, r_fractions2.size() );

        CHECK_EQUAL( "X-Wild-b0-c0:X-Wild-b0-c0", r_allele_names2[ 12 ] );
        CHECK_EQUAL( "X-Wild-b0-c0:X-Wild-b1-c0", r_allele_names2[ 13 ] );
        CHECK_EQUAL( "X-Wild-b0-c0:X-Wild-b0-c1", r_allele_names2[ 14 ] );
        CHECK_EQUAL( "X-Wild-b0-c0:X-Wild-b1-c1", r_allele_names2[ 15 ] );
        CHECK_EQUAL( "X-Wild-b1-c0:X-Wild-b1-c0", r_allele_names2[ 24 ] );
        CHECK_EQUAL( "X-Wild-b1-c0:X-Wild-b1-c1", r_allele_names2[ 25 ] );
        CHECK_EQUAL( "X-Wild-b0-c1:X-Wild-b0-c1", r_allele_names2[ 35 ] );
        CHECK_EQUAL( "X-Wild-b0-c1:X-Wild-b1-c1", r_allele_names2[ 36 ] );
        CHECK_EQUAL( "X-Wild-b1-c1:X-Wild-b1-c1", r_allele_names2[ 43 ] );

        CHECK_CLOSE( 0.027225, r_fractions2[ 12 ], 0.00001 );
        CHECK_CLOSE( 0.093194, r_fractions2[ 13 ], 0.00001 );
        CHECK_CLOSE( 0.045026, r_fractions2[ 14 ], 0.00001 );
        CHECK_CLOSE( 0.212565, r_fractions2[ 15 ], 0.00001 );
        CHECK_CLOSE( 0.153927, r_fractions2[ 24 ], 0.00001 );
        CHECK_CLOSE( 0.225131, r_fractions2[ 25 ], 0.00001 );
        CHECK_CLOSE( 0.018848, r_fractions2[ 35 ], 0.00001 );
        CHECK_CLOSE( 0.093194, r_fractions2[ 36 ], 0.00001 );
        CHECK_CLOSE( 0.130890, r_fractions2[ 43 ], 0.00001 );
    }

    TEST_FIXTURE(VectorCounterFixture, TestInvalidSampleSize )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorCounterTest/TestConfigureInvalidSampleSize.json",
            "Configuration variable 'Sample_Size_Gaussian_Std_Dev' with value -200 out of range: less than 1.17549e-38.");
    }

    TEST_FIXTURE(VectorCounterFixture, TestInvalidSpecies )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorCounterTest/TestConfigureInvalidSpecies.json",
            "Constrained String (Species) with specified value 'SillySkeeter' invalid. Possible values are: \narabiensis\nfunestus\ngambiae");
    }

    TEST_FIXTURE(VectorCounterFixture, TestInvalidUpdatePeriod)
    {
        TestHelper_ConfigureException(__LINE__, "testdata/VectorCounterTest/TestConfigureInvalidUpdatePeriod.json",
            "Configuration variable 'Update_Period' with value -0.4 out of range: less than 0.");
    }

    TEST_FIXTURE(VectorCounterFixture, TestInvalidGender)
    {
        TestHelper_ConfigureException(__LINE__, "testdata/VectorCounterTest/TestConfigureInvalidGender.json",
            "Failed to find enum match for value 'FEMALE' and key 'Gender'.\nPossible values are:\nVECTOR_FEMALE\nVECTOR_MALE\nVECTOR_BOTH_GENDERS");
    }

    TEST_FIXTURE(VectorCounterFixture, TestInvalidCounterType)
    {
        TestHelper_ConfigureException(__LINE__, "testdata/VectorCounterTest/TestConfigureInvalidCounterType.json",
            "Failed to find enum match for value 'FREQUENCY' and key 'Count_Type'.\nPossible values are:\nALLELE_FREQ\nGENOME_FRACTION");
    }

}
