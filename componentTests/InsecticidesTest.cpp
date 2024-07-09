
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "Insecticides.h"
#include "SimulationConfig.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "Configuration.h"

using namespace Kernel;

SUITE( InsecticidesTest )
{
    struct InsecticidesFixture
    {
        SimulationConfig* m_pSimulationConfig;

        int m_SpeciesIndexArabiensis;
        int m_SpeciesIndexFunestus;
        int m_SpeciesIndexGambiae;

        VectorGenome m_Genome_a0b0_a0b0;
        VectorGenome m_Genome_a0b0_a0b1;
        VectorGenome m_Genome_a0b1_a0b0;
        VectorGenome m_Genome_a0b1_a0b1;

        VectorGenome m_Genome_a0b0_a1b0;
        VectorGenome m_Genome_a0b0_a1b1;
        VectorGenome m_Genome_a0b1_a1b0;
        VectorGenome m_Genome_a0b1_a1b1;

        VectorGenome m_Genome_a1b0_a0b0;
        VectorGenome m_Genome_a1b0_a0b1;
        VectorGenome m_Genome_a1b1_a0b0;
        VectorGenome m_Genome_a1b1_a0b1;

        VectorGenome m_Genome_a1b0_a1b0;
        VectorGenome m_Genome_a1b0_a1b1;
        VectorGenome m_Genome_a1b1_a1b0;
        VectorGenome m_Genome_a1b1_a1b1;

        VectorGenome m_Genome_c0_c0;
        VectorGenome m_Genome_c0_c1;
        VectorGenome m_Genome_c1_c0;
        VectorGenome m_Genome_c1_c1;

        VectorGenome m_Genome_d0_d0;
        VectorGenome m_Genome_d0_d1;
        VectorGenome m_Genome_d1_d0;
        VectorGenome m_Genome_d1_d1;

        InsecticidesFixture()
            : m_pSimulationConfig( new SimulationConfig() )
            , m_SpeciesIndexArabiensis(0)
            , m_SpeciesIndexFunestus(0)
            , m_SpeciesIndexGambiae(0)
            , m_Genome_a0b0_a0b0()
            , m_Genome_a0b0_a0b1()
            , m_Genome_a0b1_a0b0()
            , m_Genome_a0b1_a0b1()
            , m_Genome_a0b0_a1b0()
            , m_Genome_a0b0_a1b1()
            , m_Genome_a0b1_a1b0()
            , m_Genome_a0b1_a1b1()
            , m_Genome_a1b0_a0b0()
            , m_Genome_a1b0_a0b1()
            , m_Genome_a1b1_a0b0()
            , m_Genome_a1b1_a0b1()
            , m_Genome_a1b0_a1b0()
            , m_Genome_a1b0_a1b1()
            , m_Genome_a1b1_a1b0()
            , m_Genome_a1b1_a1b1()
            , m_Genome_c0_c0()
            , m_Genome_c0_c1()
            , m_Genome_c1_c0()
            , m_Genome_c1_c1()
            , m_Genome_d0_d0()
            , m_Genome_d0_d1()
            , m_Genome_d1_d0()
            , m_Genome_d1_d1()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            try
            {
                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/InsecticidesTest/config.json" ) );
                m_pSimulationConfig->vector_params->vector_species.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params" );
                m_pSimulationConfig->vector_params->vector_species.CheckConfiguration();
            }
            catch( DetailedException& re )
            {
                PrintDebug( re.GetMsg() );
                throw re;
            }

            m_SpeciesIndexArabiensis = m_pSimulationConfig->vector_params->vector_species.GetSpecies( "arabiensis" ).index;
            m_SpeciesIndexFunestus   = m_pSimulationConfig->vector_params->vector_species.GetSpecies( "funestus"   ).index;
            m_SpeciesIndexGambiae    = m_pSimulationConfig->vector_params->vector_species.GetSpecies( "gambiae"    ).index;

            m_Genome_a0b0_a0b0.SetLocus( 0, 0, 0 );
            m_Genome_a0b0_a0b0.SetLocus( 1, 0, 0 );
            m_Genome_a0b0_a0b0.SetLocus( 2, 0, 0 );

            m_Genome_a0b0_a0b1.SetLocus( 0, 0, 0 );
            m_Genome_a0b0_a0b1.SetLocus( 1, 0, 0 );
            m_Genome_a0b0_a0b1.SetLocus( 2, 0, 1 );

            m_Genome_a0b1_a0b0.SetLocus( 0, 0, 0 );
            m_Genome_a0b1_a0b0.SetLocus( 1, 0, 0 );
            m_Genome_a0b1_a0b0.SetLocus( 2, 1, 0 );

            m_Genome_a0b1_a0b1.SetLocus( 0, 0, 0 );
            m_Genome_a0b1_a0b1.SetLocus( 1, 0, 0 );
            m_Genome_a0b1_a0b1.SetLocus( 2, 1, 1 );

            m_Genome_a0b0_a1b0.SetLocus( 0, 0, 0 );
            m_Genome_a0b0_a1b0.SetLocus( 1, 0, 1 );
            m_Genome_a0b0_a1b0.SetLocus( 2, 0, 0 );

            m_Genome_a0b0_a1b1.SetLocus( 0, 0, 0 );
            m_Genome_a0b0_a1b1.SetLocus( 1, 0, 1 );
            m_Genome_a0b0_a1b1.SetLocus( 2, 0, 1 );

            m_Genome_a0b1_a1b0.SetLocus( 0, 0, 0 );
            m_Genome_a0b1_a1b0.SetLocus( 1, 0, 1 );
            m_Genome_a0b1_a1b0.SetLocus( 2, 1, 0 );

            m_Genome_a0b1_a1b1.SetLocus( 0, 0, 0 );
            m_Genome_a0b1_a1b1.SetLocus( 1, 0, 1 );
            m_Genome_a0b1_a1b1.SetLocus( 2, 1, 1 );

            m_Genome_a1b0_a0b0.SetLocus( 0, 0, 0 );
            m_Genome_a1b0_a0b0.SetLocus( 1, 1, 0 );
            m_Genome_a1b0_a0b0.SetLocus( 2, 0, 0 );

            m_Genome_a1b0_a0b1.SetLocus( 0, 0, 0 );
            m_Genome_a1b0_a0b1.SetLocus( 1, 1, 0 );
            m_Genome_a1b0_a0b1.SetLocus( 2, 0, 1 );

            m_Genome_a1b1_a0b0.SetLocus( 0, 0, 0 );
            m_Genome_a1b1_a0b0.SetLocus( 1, 1, 0 );
            m_Genome_a1b1_a0b0.SetLocus( 2, 1, 0 );

            m_Genome_a1b1_a0b1.SetLocus( 0, 0, 0 );
            m_Genome_a1b1_a0b1.SetLocus( 1, 1, 0 );
            m_Genome_a1b1_a0b1.SetLocus( 2, 1, 1 );

            m_Genome_a1b0_a1b0.SetLocus( 0, 0, 0 );
            m_Genome_a1b0_a1b0.SetLocus( 1, 1, 1 );
            m_Genome_a1b0_a1b0.SetLocus( 2, 0, 0 );

            m_Genome_a1b0_a1b1.SetLocus( 0, 0, 0 );
            m_Genome_a1b0_a1b1.SetLocus( 1, 1, 1 );
            m_Genome_a1b0_a1b1.SetLocus( 2, 0, 1 );

            m_Genome_a1b1_a1b0.SetLocus( 0, 0, 0 );
            m_Genome_a1b1_a1b0.SetLocus( 1, 1, 1 );
            m_Genome_a1b1_a1b0.SetLocus( 2, 1, 0 );

            m_Genome_a1b1_a1b1.SetLocus( 0, 0, 0 );
            m_Genome_a1b1_a1b1.SetLocus( 1, 1, 1 );
            m_Genome_a1b1_a1b1.SetLocus( 2, 1, 1 );

            m_Genome_c0_c0.SetLocus( 0, 0, 0 );
            m_Genome_c0_c0.SetLocus( 1, 0, 0 );

            m_Genome_c0_c1.SetLocus( 0, 0, 0 );
            m_Genome_c0_c1.SetLocus( 1, 0, 1 );

            m_Genome_c1_c0.SetLocus( 0, 0, 0 );
            m_Genome_c1_c0.SetLocus( 1, 1, 0 );

            m_Genome_c1_c1.SetLocus( 0, 0, 0 );
            m_Genome_c1_c1.SetLocus( 1, 1, 1 );

            m_Genome_d0_d0.SetLocus( 0, 0, 0 );
            m_Genome_d0_d0.SetLocus( 1, 0, 0 );

            m_Genome_d0_d1.SetLocus( 0, 0, 0 );
            m_Genome_d0_d1.SetLocus( 1, 0, 1 );

            m_Genome_d1_d0.SetLocus( 0, 0, 0 );
            m_Genome_d1_d0.SetLocus( 1, 1, 0 );

            m_Genome_d1_d1.SetLocus( 0, 0, 0 );
            m_Genome_d1_d1.SetLocus( 1, 1, 1 );

            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;
            JsonConfigurable::_track_missing = true;
        }

        ~InsecticidesFixture()
        {
            delete m_pSimulationConfig;
            Environment::setSimulationConfig( nullptr );
            Environment::Finalize();
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = true;
            JsonConfigurable::_track_missing = true;
        }

        void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename.c_str() ) );

            InsecticideCollection collection( &(m_pSimulationConfig->vector_params->vector_species) );
            try
            {
                collection.ConfigureFromJsonAndKey( p_config.get(), "Insecticides" );
                collection.CheckConfiguration();

                CHECK_LN( false, lineNumber ); // should not get here
            }
            catch( DetailedException& re )
            {
                std::string msg = re.GetMsg();
                if( msg.find( rExpMsg ) == string::npos )
                {
                    PrintDebug( rExpMsg );
                    PrintDebug( msg );
                    CHECK_LN( false, lineNumber );
                }
            }
        }
    };

    TEST_FIXTURE( InsecticidesFixture, TestConfigure )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/InsecticidesTest/TestConfigure.json" ) );

        InsecticideCollection collection( &(m_pSimulationConfig->vector_params->vector_species) );
        try
        {
            collection.ConfigureFromJsonAndKey( p_config.get(), "Insecticides" );
            collection.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        CHECK_EQUAL( 3, collection.Size() );

        jsonConfigurable::tDynamicStringSet insecticide_names = collection.GetInsecticideNames();
        CHECK( insecticide_names.find( "pyrethroid"      ) != insecticide_names.end() );
        CHECK( insecticide_names.find( "carbamate"       ) != insecticide_names.end() );
        CHECK( insecticide_names.find( "organophosphate" ) != insecticide_names.end() );

        const Insecticide* p_pyrethroid = collection.GetInsecticide( "pyrethroid" );
        CHECK_EQUAL( "pyrethroid", p_pyrethroid->GetName() );
        GeneticProbability gp_pyrethroid = p_pyrethroid->GetResistance( ResistanceType::KILLING );
        CHECK_CLOSE( 1.0, gp_pyrethroid.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b0_a0b0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.6, gp_pyrethroid.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b1_a0b0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.6, gp_pyrethroid.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b0_a0b1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.6, gp_pyrethroid.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b1_a0b1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4, gp_pyrethroid.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b0_a1b0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4, gp_pyrethroid.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b1_a1b0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4, gp_pyrethroid.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b0_a1b1 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_pyrethroid.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b1_a1b1 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_pyrethroid.GetValue( m_SpeciesIndexFunestus,   m_Genome_c0_c0     ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_pyrethroid.GetValue( m_SpeciesIndexFunestus,   m_Genome_c1_c0     ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_pyrethroid.GetValue( m_SpeciesIndexFunestus,   m_Genome_c0_c1     ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_pyrethroid.GetValue( m_SpeciesIndexFunestus,   m_Genome_c1_c1     ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_pyrethroid.GetValue( m_SpeciesIndexGambiae,    m_Genome_d0_d0     ), FLT_EPSILON );
        CHECK_CLOSE( 0.5, gp_pyrethroid.GetValue( m_SpeciesIndexGambiae,    m_Genome_d1_d0     ), FLT_EPSILON );
        CHECK_CLOSE( 0.5, gp_pyrethroid.GetValue( m_SpeciesIndexGambiae,    m_Genome_d0_d1     ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_pyrethroid.GetValue( m_SpeciesIndexGambiae,    m_Genome_d1_d1     ), FLT_EPSILON );

        const Insecticide* p_carbamate = collection.GetInsecticide( "carbamate" );
        CHECK_EQUAL( "carbamate", p_carbamate->GetName() );
        GeneticProbability gp_carbamate = p_carbamate->GetResistance( ResistanceType::KILLING );
        CHECK_CLOSE( 1.0, gp_carbamate.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b0_a0b0 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_carbamate.GetValue( m_SpeciesIndexFunestus,   m_Genome_c0_c0     ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_carbamate.GetValue( m_SpeciesIndexFunestus,   m_Genome_c1_c0     ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_carbamate.GetValue( m_SpeciesIndexFunestus,   m_Genome_c0_c1     ), FLT_EPSILON );
        CHECK_CLOSE( 1.5, gp_carbamate.GetValue( m_SpeciesIndexFunestus,   m_Genome_c1_c1     ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_carbamate.GetValue( m_SpeciesIndexGambiae,    m_Genome_d1_d1     ), FLT_EPSILON );

        const Insecticide* p_org = collection.GetInsecticide( "organophosphate" );
        CHECK_EQUAL( "organophosphate", p_org->GetName() );
        GeneticProbability gp_org = p_org->GetResistance( ResistanceType::KILLING );
        CHECK_CLOSE( 1.0, gp_org.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b0_a0b0 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_org.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b1_a0b0 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_org.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b0_a0b1 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_org.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b1_a0b1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.1, gp_org.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b0_a1b0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.1, gp_org.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b1_a1b0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.1, gp_org.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b0_a1b1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.1, gp_org.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b1_a1b1 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_org.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b1_a0b0 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_org.GetValue( m_SpeciesIndexFunestus,   m_Genome_c0_c0     ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp_org.GetValue( m_SpeciesIndexGambiae,    m_Genome_d1_d1     ), FLT_EPSILON );

        // ------------------------------------------------------------------------------------
        // --- Pretend that we are intervention with 0.7 effectivity
        // --- We multiply the effectivity times our resistance to get the current effectivity
        // ------------------------------------------------------------------------------------
        float intervention_effectivity = 0.7f;
        GeneticProbability gp_pyrethroid_effect = gp_pyrethroid * intervention_effectivity;
        CHECK_CLOSE( 0.70f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b0_a0b0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.42f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b1_a0b0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.42f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b0_a0b1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.42f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexArabiensis, m_Genome_a0b1_a0b1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.28f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b0_a1b0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.28f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b1_a1b0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.28f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b0_a1b1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.70f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexArabiensis, m_Genome_a1b1_a1b1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.70f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexFunestus,   m_Genome_c0_c0     ), FLT_EPSILON );
        CHECK_CLOSE( 0.70f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexFunestus,   m_Genome_c1_c0     ), FLT_EPSILON );
        CHECK_CLOSE( 0.70f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexFunestus,   m_Genome_c0_c1     ), FLT_EPSILON );
        CHECK_CLOSE( 0.70f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexFunestus,   m_Genome_c1_c1     ), FLT_EPSILON );
        CHECK_CLOSE( 0.70f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexGambiae,    m_Genome_d0_d0     ), FLT_EPSILON );
        CHECK_CLOSE( 0.35f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexGambiae,    m_Genome_d1_d0     ), FLT_EPSILON );
        CHECK_CLOSE( 0.35f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexGambiae,    m_Genome_d0_d1     ), FLT_EPSILON );
        CHECK_CLOSE( 0.70f, gp_pyrethroid_effect.GetValue( m_SpeciesIndexGambiae,    m_Genome_d1_d1     ), FLT_EPSILON );
    }

    TEST_FIXTURE( InsecticidesFixture, TestAlleleComboProbabilityConfigCopyConstructor )
    {
        std::stringstream ss;
        ss << "{";
        ss << "    \"Species\" : \"funestus\",";
        ss << "    \"Allele_Combinations\" : [";
        ss << "        [ \"c1\", \"*\" ]";
        ss << "    ],";
        ss << "    \"Larval_Killing_Modifier\": 1.0,";
        ss << "    \"Blocking_Modifier\": 0.75,";
        ss << "    \"Killing_Modifier\": 0.25";
        ss << "}";

        Configuration* input_json = Configuration::Load( ss, "Hardcoded" );

        AlleleComboProbabilityConfig acp_config_base( &(m_pSimulationConfig->vector_params->vector_species) );
        acp_config_base.Configure( input_json );

        CHECK_EQUAL( 1.00, acp_config_base.GetProbability( ResistanceType::LARVAL_KILLING ).GetValue() );
        CHECK_EQUAL( 0.75, acp_config_base.GetProbability( ResistanceType::BLOCKING       ).GetValue() );
        CHECK_EQUAL( 0.25, acp_config_base.GetProbability( ResistanceType::KILLING        ).GetValue() );

        AlleleComboProbabilityConfig acp_config_copy( acp_config_base );

        CHECK_EQUAL( 1.00, acp_config_copy.GetProbability( ResistanceType::LARVAL_KILLING ).GetValue() );
        CHECK_EQUAL( 0.75, acp_config_copy.GetProbability( ResistanceType::BLOCKING       ).GetValue() );
        CHECK_EQUAL( 0.25, acp_config_copy.GetProbability( ResistanceType::KILLING        ).GetValue() );
    }

    TEST_FIXTURE( InsecticidesFixture, TestAlleleCombosNotDefined )
    {
        std::stringstream ss;
        ss << "The 'Insecticides' configuration is invalid.\n";
        ss << "The following genomes are ambiguous and need to be configured:\n";
        ss << "For species 'arabiensis':\n";
        ss << "X-a0-b0:X-a0-b0, \n";
        ss << "X-a1-b0:X-a0-b0, \n";
        ss << "X-a0-b1:X-a0-b0, \n";
        ss << "X-a1-b1:X-a0-b0, \n";
        ss << "X-a0-b0:X-a1-b0, \n";
        ss << "X-a0-b1:X-a1-b0, \n";
        ss << "X-a0-b0:X-a0-b1, \n";
        ss << "X-a1-b0:X-a0-b1, \n";
        ss << "X-a0-b0:X-a1-b1\n";
        ss << "\n";
        ss << "For species 'funestus':\n";
        ss << "X-c1:X-c0, \n";
        ss << "X-c0:X-c1\n";

        TestHelper_ConfigureException( __LINE__, "testdata/InsecticidesTest/TestAlleleCombosNotDefined.json",
                                       ss.str() );
    }

    TEST_FIXTURE( InsecticidesFixture, TestNoInsecticideName )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/InsecticidesTest/TestNoInsecticideName.json",
                                       "'Insecticides.Name' cannot be empty string." );
    }

    TEST_FIXTURE( InsecticidesFixture, TestMissingInsecticideName )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/InsecticidesTest/TestMissingInsecticideName.json",
                                       "Parameter 'Insecticides.Name' not found in input file 'testdata/InsecticidesTest/TestMissingInsecticideName.json'." );
    }

    TEST_FIXTURE( InsecticidesFixture, TestDuplicateInsecticideNames )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/InsecticidesTest/TestDuplicateInsecticideNames.json",
                                       "Duplicate insecticide name.\nThe names of the insecticides in 'Insecticides' must be unique.\nThe following names are defined:\npyrethroid\npyrethroid\norganophosphate" );
    }
}
