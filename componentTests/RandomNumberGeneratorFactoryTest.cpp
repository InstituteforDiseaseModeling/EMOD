/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"
#include "RandomNumberGeneratorFactory.h"
#include "RANDOM.h"

using namespace Kernel;


SUITE( RandomNumberGeneratorFactoryTest )
{
    struct RngFixture
    {
        RngFixture()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
        }

        ~RngFixture()
        {
            Environment::Finalize();
        }

        std::vector<uint32_t> CreateNodeIds()
        {
            std::vector<uint32_t> node_ids;

            node_ids.push_back( 11 );
            node_ids.push_back( 22 );
            node_ids.push_back( 33 );
            node_ids.push_back( 44 );
            node_ids.push_back( 55 );
            node_ids.push_back( 66 );
            node_ids.push_back( 77 );
            node_ids.push_back( 88 );
            node_ids.push_back( 99 );

            return node_ids;
        }

        void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
        {
            try
            {
                EnvPtr->Config = Environment::LoadConfigurationFile( rFilename );

                RandomNumberGeneratorFactory factory;
                factory.Configure( EnvPtr->Config );

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

    TEST_FIXTURE( RngFixture, TestOnePerCore )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/RandomNumberGeneratorFactoryTest/OnePerCore.json" );

        RandomNumberGeneratorFactory factory;
        factory.Configure( EnvPtr->Config );

        std::vector<uint32_t> node_ids = CreateNodeIds();
        factory.SetNodeIds( node_ids );

        unique_ptr<RANDOMBASE> p_rng_sim( factory.CreateRng() );
        CHECK( p_rng_sim != nullptr );

        for( auto node_id : node_ids )
        {
            unique_ptr<RANDOMBASE> p_rng_node( factory.CreateRng( node_id ) );
            CHECK( p_rng_node == nullptr );
        }
    }

    TEST_FIXTURE( RngFixture, TestOnePerNode )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/RandomNumberGeneratorFactoryTest/OnePerNode.json" );

        RandomNumberGeneratorFactory factory;
        factory.Configure( EnvPtr->Config );

        std::vector<uint32_t> node_ids = CreateNodeIds();
        factory.SetNodeIds( node_ids );

        unique_ptr<RANDOMBASE> p_rng_sim( factory.CreateRng( 0 ) );
        CHECK( p_rng_sim == nullptr );

        unique_ptr<RANDOMBASE> p_rng_node_11( factory.CreateRng( 11 ) );
        unique_ptr<RANDOMBASE> p_rng_node_44( factory.CreateRng( 44 ) );
        unique_ptr<RANDOMBASE> p_rng_node_77( factory.CreateRng( 77 ) );
        unique_ptr<RANDOMBASE> p_rng_node_99( factory.CreateRng( 99 ) );
        CHECK( p_rng_node_11 != nullptr );
        CHECK( p_rng_node_44 != nullptr );
        CHECK( p_rng_node_77 != nullptr );
        CHECK( p_rng_node_99 != nullptr );
    }

    TEST_FIXTURE( RngFixture, TestInvalidLcgOnePerNode )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/RandomNumberGeneratorFactoryTest/TestInvalidLcgOnePerNode.json",
                                       "Variable or parameter 'Random_Number_Generator_Type' with value USE_LINEAR_CONGRUENTIAL is incompatible with variable or parameter 'Random_Number_Generator_Policy' with value ONE_PER_NODE. \n'Random_Number_Generator_Policy' must be 'ONE_PER_CORE' when using 'Random_Number_Generator_Type' = 'USE_LINEAR_CONGRUENTIAL'." );
    }

    TEST_FIXTURE( RngFixture, TestInvalidAllowNodeIDZero )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/RandomNumberGeneratorFactoryTest/TestInvalidAllowNodeIDZero.json",
                                       "Variable or parameter 'Allow_NodeID_Zero' with value 1 is incompatible with variable or parameter 'Random_Number_Generator_Policy' with value ONE_PER_NODE. \n'Random_Number_Generator_Policy' must be 'ONE_PER_CORE' when using 'Allow_NodeID_Zero' = 1." );
    }
}