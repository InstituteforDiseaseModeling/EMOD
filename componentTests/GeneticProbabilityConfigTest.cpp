#include "stdafx.h"
#include "UnitTest++.h"
#include <string>

#include "componentTests.h"
#include "GeneticProbabilityConfig.h"
#include "VectorSpeciesParameters.h"

using namespace Kernel;

SUITE(GeneticProbabilityConfigTest)
{
    struct GeneticProbabilityConfigFixture
    {
        VectorSpeciesCollection m_VectorSpeciesParamsCollection;

        GeneticProbabilityConfigFixture()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );

            try
            {
                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/GeneticProbabilityConfigTest/config.json" ) );
                m_VectorSpeciesParamsCollection.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params" );
                m_VectorSpeciesParamsCollection.CheckConfiguration();
            }
            catch( DetailedException& re )
            {
                PrintDebug( re.GetMsg() );
                throw re;
            }
        }

        ~GeneticProbabilityConfigFixture()
        {
            Environment::Finalize();
        }
    };

    TEST_FIXTURE(GeneticProbabilityConfigFixture, TestReadSingleSpecies)
    {
        VectorGenome genome_a0b0_a0b0;
        genome_a0b0_a0b0.SetLocus( 0, 0, 0 );
        genome_a0b0_a0b0.SetLocus( 1, 0, 0 );
        genome_a0b0_a0b0.SetLocus( 2, 0, 0 );

        VectorGenome genome_a0b0_a1b1;
        genome_a0b0_a1b1.SetLocus( 0, 0, 0 );
        genome_a0b0_a1b1.SetLocus( 1, 0, 1 );
        genome_a0b0_a1b1.SetLocus( 2, 0, 1 );

        VectorGenome genome_a1b0_a1b1;
        genome_a1b0_a1b1.SetLocus( 0, 0, 0 );
        genome_a1b0_a1b1.SetLocus( 1, 1, 1 );
        genome_a1b0_a1b1.SetLocus( 2, 0, 1 );

        GeneticProbabilityConfig gpc( m_VectorSpeciesParamsCollection[ 0 ] );

        try
        {
            std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/GeneticProbabilityConfigTest/TestReadSingleSpecies.json" ) );
            gpc.Configure( p_config.get() );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        GeneticProbability gp = gpc.GetProbability();
        CHECK_EQUAL( 0.5, gp.GetDefaultValue() );
        CHECK_CLOSE( 0.5, gp.GetValue( 0, genome_a0b0_a0b0 ), FLT_EPSILON ); // not specified so default
        CHECK_CLOSE( 0.8, gp.GetValue( 0, genome_a0b0_a1b1 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp.GetValue( 0, genome_a1b0_a1b1 ), FLT_EPSILON );
    }

    TEST_FIXTURE(GeneticProbabilityConfigFixture, TestReadMultipleSpecies)
    {
        VectorGenome genome_0_a0b0_a0b0;
        genome_0_a0b0_a0b0.SetLocus( 0, 0, 0 );
        genome_0_a0b0_a0b0.SetLocus( 1, 0, 0 );
        genome_0_a0b0_a0b0.SetLocus( 2, 0, 0 );

        VectorGenome genome_0_a0b0_a1b1;
        genome_0_a0b0_a1b1.SetLocus( 0, 0, 0 );
        genome_0_a0b0_a1b1.SetLocus( 1, 0, 1 );
        genome_0_a0b0_a1b1.SetLocus( 2, 0, 1 );

        VectorGenome genome_0_a1b0_a1b1;
        genome_0_a1b0_a1b1.SetLocus( 0, 0, 0 );
        genome_0_a1b0_a1b1.SetLocus( 1, 1, 1 );
        genome_0_a1b0_a1b1.SetLocus( 2, 0, 1 );

        VectorGenome genome_0_a0b1_a1b1;
        genome_0_a0b1_a1b1.SetLocus( 0, 0, 0 );
        genome_0_a0b1_a1b1.SetLocus( 1, 0, 1 );
        genome_0_a0b1_a1b1.SetLocus( 2, 1, 1 );


        VectorGenome genome_1_c0_c0;
        genome_1_c0_c0.SetLocus( 0, 0, 0 );
        genome_1_c0_c0.SetLocus( 1, 0, 0 );

        VectorGenome genome_1_c1_c0;
        genome_1_c1_c0.SetLocus( 0, 0, 0 );
        genome_1_c1_c0.SetLocus( 1, 1, 0 );

        VectorGenome genome_1_c0_c1;
        genome_1_c0_c1.SetLocus( 0, 0, 0 );
        genome_1_c0_c1.SetLocus( 1, 0, 1 );

        VectorGenome genome_1_c1_c1;
        genome_1_c1_c1.SetLocus( 0, 0, 0 );
        genome_1_c1_c1.SetLocus( 1, 1, 1 );


        VectorGenome genome_2_d0_d0;
        genome_2_d0_d0.SetLocus( 0, 0, 0 );
        genome_2_d0_d0.SetLocus( 1, 0, 0 );

        VectorGenome genome_2_d1_d0;
        genome_2_d1_d0.SetLocus( 0, 0, 0 );
        genome_2_d1_d0.SetLocus( 1, 1, 0 );

        VectorGenome genome_2_d0_d1;
        genome_2_d0_d1.SetLocus( 0, 0, 0 );
        genome_2_d0_d1.SetLocus( 1, 0, 1 );

        VectorGenome genome_2_d1_d1;
        genome_2_d1_d1.SetLocus( 0, 0, 0 );
        genome_2_d1_d1.SetLocus( 1, 1, 1 );

        GeneticProbabilityConfig gpc( &m_VectorSpeciesParamsCollection );
        try
        {
            std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/GeneticProbabilityConfigTest/TestReadMultipleSpecies.json" ) );
            gpc.Configure( p_config.get() );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        GeneticProbability gp = gpc.GetProbability();
        CHECK_CLOSE( 0.2, gp.GetDefaultValue(), FLT_EPSILON );

        CHECK_CLOSE( 0.2, gp.GetValue( 0, genome_0_a0b0_a0b0 ), FLT_EPSILON ); // not specified so default
        CHECK_CLOSE( 0.2, gp.GetValue( 0, genome_0_a0b0_a1b1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.8, gp.GetValue( 0, genome_0_a0b1_a1b1 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, gp.GetValue( 0, genome_0_a1b0_a1b1 ), FLT_EPSILON );

        CHECK_CLOSE( 0.2, gp.GetValue( 1, genome_1_c0_c0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.2, gp.GetValue( 1, genome_1_c1_c0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.2, gp.GetValue( 1, genome_1_c0_c1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.6, gp.GetValue( 1, genome_1_c1_c1 ), FLT_EPSILON );

        CHECK_CLOSE( 0.2, gp.GetValue( 2, genome_2_d0_d0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.2, gp.GetValue( 2, genome_2_d1_d0 ), FLT_EPSILON );
        CHECK_CLOSE( 0.2, gp.GetValue( 2, genome_2_d0_d1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.4, gp.GetValue( 2, genome_2_d1_d1 ), FLT_EPSILON );
    }
}
