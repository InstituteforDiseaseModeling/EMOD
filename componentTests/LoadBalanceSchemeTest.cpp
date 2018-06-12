/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "Configure.h"
#include "Environment.h"
#include "LoadBalanceScheme.h"

using namespace Kernel; 


SUITE(LoadBalanceSchemeTest)
{
    struct Fixture
    {
        Fixture()
        {
            JsonConfigurable::ClearMissingParameters();

            Environment::Finalize();
            Environment::setLogger( new SimpleLogger() );
        }

        ~Fixture()
        {
            Environment::Finalize();
        }
    };


    TEST_FIXTURE(Fixture, TestCreateLegacy)
    {
        std::string filename =  "testdata/LoadBalanceSchemeTest/Madagascar_2_5arcmin_load_balancing_comm.bin" ;
        unique_ptr<IInitialLoadBalanceScheme> p_lbs( LoadBalanceSchemeFactory::Create( filename, 30145, 16 ) );

        CHECK_EQUAL(  0, p_lbs->GetInitialRankFromNodeId( 360515410 ) );
        CHECK_EQUAL(  0, p_lbs->GetInitialRankFromNodeId( 360580946 ) );
        CHECK_EQUAL(  0, p_lbs->GetInitialRankFromNodeId( 360449873 ) );
        CHECK_EQUAL(  5, p_lbs->GetInitialRankFromNodeId( 357697225 ) );
        CHECK_EQUAL(  5, p_lbs->GetInitialRankFromNodeId( 357762761 ) );
        CHECK_EQUAL(  5, p_lbs->GetInitialRankFromNodeId( 357828297 ) );
        CHECK_EQUAL(  8, p_lbs->GetInitialRankFromNodeId( 355272352 ) );
        CHECK_EQUAL(  8, p_lbs->GetInitialRankFromNodeId( 355337888 ) );
        CHECK_EQUAL( 10, p_lbs->GetInitialRankFromNodeId( 356386421 ) );
        CHECK_EQUAL( 10, p_lbs->GetInitialRankFromNodeId( 356451957 ) );
        CHECK_EQUAL( 13, p_lbs->GetInitialRankFromNodeId( 354092619 ) );
        CHECK_EQUAL( 13, p_lbs->GetInitialRankFromNodeId( 354158155 ) );
        CHECK_EQUAL( 14, p_lbs->GetInitialRankFromNodeId( 356320828 ) );
        CHECK_EQUAL( 14, p_lbs->GetInitialRankFromNodeId( 356386364 ) );
        CHECK_EQUAL( 15, p_lbs->GetInitialRankFromNodeId( 354420234 ) );
        CHECK_EQUAL( 15, p_lbs->GetInitialRankFromNodeId( 354485770 ) );

        // Check unknown node_id
        CHECK_EQUAL( 0,  p_lbs->GetInitialRankFromNodeId( 9999 ) );
    }

    TEST_FIXTURE(Fixture, TestCreateJson)
    {

        float node_ids_0[] = { 1,2,3,4,5,11,12,13,14,15,21,22,23,24,25,31,32,33,34,35,45} ;
        float node_ids_1[] = {6,7,8,9,10,16,17,18,19,20,26,27,28,29,30,36,37,38,39,40,46,47,48,49,50,56,57,58,59,60,67,68,69,70};
        float node_ids_2[] = {61,62,63,64,65,66,71,72,73,74,75,76,77,81,82,83,84,85,86,87,91,92,93,94,95,96,97};
        float node_ids_3[] = {41,42,43,44,51,52,53,54,55,78,79,80,88,89,90,98,99,100} ;

        int num_node_ids_0  = 21 ;
        int num_node_ids_1  = 34 ;
        int num_node_ids_2  = 27 ;
        int num_node_ids_3  = 18 ;

        std::string filename =  "testdata/LoadBalanceSchemeTest/TestCreateJson.json" ;
        unique_ptr<IInitialLoadBalanceScheme> p_lbs( LoadBalanceSchemeFactory::Create( filename, 100, 4 ) );

        for( int i = 0 ; i < num_node_ids_0 ; i++ )
        {
            CHECK_EQUAL( 0, p_lbs->GetInitialRankFromNodeId( node_ids_0[i] ) );
        }
        for( int i = 0 ; i < num_node_ids_1 ; i++ )
        {
            CHECK_EQUAL( 1, p_lbs->GetInitialRankFromNodeId( node_ids_1[i] ) );
        }
        for( int i = 0 ; i < num_node_ids_2 ; i++ )
        {
            CHECK_EQUAL( 2, p_lbs->GetInitialRankFromNodeId( node_ids_2[i] ) );
        }
        for( int i = 0 ; i < num_node_ids_3 ; i++ )
        {
            CHECK_EQUAL( 3, p_lbs->GetInitialRankFromNodeId( node_ids_3[i] ) );
        }

        // Check unknown node_id
        CHECK_EQUAL( 0,  p_lbs->GetInitialRankFromNodeId( 9999 ) );
    }

    TEST_FIXTURE(Fixture, TestCreateCheckerboard)
    {
        std::string filename = "" ; // no file name implies checkerboard
        unique_ptr<IInitialLoadBalanceScheme> p_lbs( LoadBalanceSchemeFactory::Create( filename, 16, 4 ) );

        CHECK_EQUAL( 0, p_lbs->GetInitialRankFromNodeId(  1 ) );
        CHECK_EQUAL( 1, p_lbs->GetInitialRankFromNodeId(  2 ) );
        CHECK_EQUAL( 2, p_lbs->GetInitialRankFromNodeId(  3 ) );
        CHECK_EQUAL( 3, p_lbs->GetInitialRankFromNodeId(  4 ) );
        CHECK_EQUAL( 0, p_lbs->GetInitialRankFromNodeId(  5 ) );
        CHECK_EQUAL( 1, p_lbs->GetInitialRankFromNodeId(  6 ) );
        CHECK_EQUAL( 2, p_lbs->GetInitialRankFromNodeId(  7 ) );
        CHECK_EQUAL( 3, p_lbs->GetInitialRankFromNodeId(  8 ) );
        CHECK_EQUAL( 0, p_lbs->GetInitialRankFromNodeId(  9 ) );
        CHECK_EQUAL( 1, p_lbs->GetInitialRankFromNodeId( 10 ) );
        CHECK_EQUAL( 2, p_lbs->GetInitialRankFromNodeId( 11 ) );
        CHECK_EQUAL( 3, p_lbs->GetInitialRankFromNodeId( 12 ) );
        CHECK_EQUAL( 0, p_lbs->GetInitialRankFromNodeId( 13 ) );
        CHECK_EQUAL( 1, p_lbs->GetInitialRankFromNodeId( 14 ) );
        CHECK_EQUAL( 2, p_lbs->GetInitialRankFromNodeId( 15 ) );
        CHECK_EQUAL( 3, p_lbs->GetInitialRankFromNodeId( 16 ) );
    }
}