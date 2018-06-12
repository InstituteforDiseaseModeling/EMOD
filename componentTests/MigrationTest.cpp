/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "common.h"
#include "IMigrationInfo.h"
#include "IMigrationInfoVector.h"
#include "SimulationConfig.h"
#include "INodeContextFake.h"
#include "IndividualHumanContextFake.h"
#include "RandomFake.h"
#include "IdmMpi.h"

using namespace Kernel; 



// maybe these shouldn't be protected in Simulation.h
typedef boost::bimap<ExternalNodeId_t, suids::suid> nodeid_suid_map_t;
typedef nodeid_suid_map_t::value_type nodeid_suid_pair;


SUITE(MigrationTest)
{
    static int m_NextId = 1 ;

    struct MigrationFixture
    {
        IdmMpi::MessageInterface* m_pMpi;
        SimulationConfig* m_pSimulationConfig ;
        RandomFake m_RandomFake ;

        MigrationFixture()
            : m_pSimulationConfig( new SimulationConfig() )
        {
            JsonConfigurable::ClearMissingParameters();

            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );

            m_pMpi = IdmMpi::MessageInterface::CreateNull();

            string configFilename("testdata/MigrationTest/config.json");
            string inputPath("testdata/MigrationTest");
            string outputPath("testdata/MigrationTest/output");
            string statePath("testdata/MigrationTest");
            string dllPath("testdata/MigrationTest");

            Environment::Initialize( m_pMpi, configFilename, inputPath, outputPath, dllPath, false );

            const_cast<Environment*>(Environment::getInstance())->RNG = &m_RandomFake;
            m_pSimulationConfig->sim_type = SimType::VECTOR_SIM ;
            m_pSimulationConfig->demographics_initial = true ;
            Environment::setSimulationConfig( m_pSimulationConfig );

        }

        ~MigrationFixture()
        {
            delete m_pMpi;
            Environment::Finalize();
        }
    };

    TEST_FIXTURE(MigrationFixture, TestBothGenders)
    {
        std::string config_filename = "testdata/MigrationTest/TestBothGenders_config.json";
        try
        {
            unique_ptr<Configuration> p_config_file( Environment::LoadConfigurationFile( config_filename.c_str() ) );

            unique_ptr<Configuration> p_config( Environment::CopyFromElement( (*p_config_file)["parameters"] ) );

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= 4 ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }


            std::string idreference = "ABC" ;
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( p_config.get(), 
                                                                                                     idreference, 
                                                                                                     SimType::GENERIC_SIM,
                                                                                                     MigrationStructure::FIXED_RATE_MIGRATION,
                                                                                                     false, 
                                                                                                     10 ) );

            CHECK( p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            // ---------------
            // --- Test Node 2
            // ---------------
            INodeContextFake nc_2( nodeid_suid_map.left.at(2) ) ;
            unique_ptr<IMigrationInfo> p_mi_2( p_mf->CreateMigrationInfo( &nc_2, nodeid_suid_map ) );

            CHECK( p_mi_2->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_2 = p_mi_2->GetReachableNodes();
            CHECK_EQUAL( 3, reachable_nodes_2.size() );
            CHECK_EQUAL( 1, reachable_nodes_2[ 0].data );
            CHECK_EQUAL( 3, reachable_nodes_2[ 1].data );
            CHECK_EQUAL( 4, reachable_nodes_2[ 2].data );

            const std::vector<MigrationType::Enum>& mig_type_list_2 = p_mi_2->GetMigrationTypes();
            CHECK_EQUAL( 3, mig_type_list_2.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 2] );

            // ================
            // === FROM NODE 2
            // ================
            m_RandomFake.SetUL( 2576980377 ); // 0.6

            IndividualHumanContextFake traveler( nullptr, &nc_2, nullptr, nullptr );

            // ------------------------------------------------------------------
            // --- Test that male < 15 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 10.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
            float trip_time = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.00001 );

            // ------------------------------------------------------------------
            // --- Test that male = 30 will migrate to node 4
            // ------------------------------------------------------------------
            traveler.SetAge( 30.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 4, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 );

            // ------------------------------------------------------------------
            // --- Test that female = 60 will migrate to node 3
            // ------------------------------------------------------------------
            traveler.SetAge( 60.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 3, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 ); //trip_time is the same because random number is the same

            // ------------------------------------------------------------------
            // --- Test that female = 90 will migrate to node 1
            // ------------------------------------------------------------------
            traveler.SetAge( 90.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            m_RandomFake.SetUL( 0 ); // 0.0 - need to change so we get the first in the list

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 1, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 15.9424, trip_time, 0.0001 ); //trip_time is different due to new random number

            // ------------------------------------------------------------------
            // --- Test that female = 120 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 120.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.0001 );

        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }


    TEST_FIXTURE(MigrationFixture, TestEachGender)
    {
        std::string config_filename = "testdata/MigrationTest/TestEachGender_config.json";
        try
        {
            unique_ptr<Configuration> p_config_file( Environment::LoadConfigurationFile( config_filename.c_str() ) );

            unique_ptr<Configuration> p_config( Environment::CopyFromElement( (*p_config_file)["parameters"] ) );

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= 4 ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }


            std::string idreference = "ABC" ;
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( p_config.get(),
                                                                                                     idreference, 
                                                                                                     SimType::GENERIC_SIM,
                                                                                                     MigrationStructure::FIXED_RATE_MIGRATION,
                                                                                                     false, 
                                                                                                     10 ) );

            CHECK( p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            // ---------------
            // --- Test Node 1
            // ---------------
            INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;
            unique_ptr<IMigrationInfo> p_mi_1( p_mf->CreateMigrationInfo( &nc_1, nodeid_suid_map ) );

            CHECK( p_mi_1->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_1 = p_mi_1->GetReachableNodes();
            CHECK_EQUAL( 3, reachable_nodes_1.size() );
            CHECK_EQUAL( 2, reachable_nodes_1[ 0].data );
            CHECK_EQUAL( 3, reachable_nodes_1[ 1].data );
            CHECK_EQUAL( 4, reachable_nodes_1[ 2].data );

            const std::vector<MigrationType::Enum>& mig_type_list_1 = p_mi_1->GetMigrationTypes();
            CHECK_EQUAL( 3, mig_type_list_1.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_1[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_1[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_1[ 2] );

            // ---------------
            // --- Test Node 2
            // ---------------
            INodeContextFake nc_2( nodeid_suid_map.left.at(2) ) ;
            unique_ptr<IMigrationInfo> p_mi_2( p_mf->CreateMigrationInfo( &nc_2, nodeid_suid_map ) );

            CHECK( p_mi_2->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_2 = p_mi_2->GetReachableNodes();
            CHECK_EQUAL( 3, reachable_nodes_2.size() );
            CHECK_EQUAL( 1, reachable_nodes_2[ 0].data );
            CHECK_EQUAL( 3, reachable_nodes_2[ 1].data );
            CHECK_EQUAL( 4, reachable_nodes_2[ 2].data );

            const std::vector<MigrationType::Enum>& mig_type_list_2 = p_mi_2->GetMigrationTypes();
            CHECK_EQUAL( 3, mig_type_list_2.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_2[ 2] );

            // ---------------
            // --- Test Node 3
            // ---------------
            INodeContextFake nc_3( nodeid_suid_map.left.at(3) ) ;
            unique_ptr<IMigrationInfo> p_mi_3( p_mf->CreateMigrationInfo( &nc_3, nodeid_suid_map ) );

            CHECK( p_mi_3->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_3 = p_mi_3->GetReachableNodes();
            CHECK_EQUAL( 3, reachable_nodes_3.size() );
            CHECK_EQUAL( 1, reachable_nodes_3[ 0].data );
            CHECK_EQUAL( 2, reachable_nodes_3[ 1].data );
            CHECK_EQUAL( 4, reachable_nodes_3[ 2].data );

            const std::vector<MigrationType::Enum>& mig_type_list_3 = p_mi_3->GetMigrationTypes();
            CHECK_EQUAL( 3, mig_type_list_3.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_3[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_3[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_3[ 2] );

            // ---------------
            // --- Test Node 4
            // ---------------
            INodeContextFake nc_4( nodeid_suid_map.left.at(4) ) ;
            unique_ptr<IMigrationInfo> p_mi_4( p_mf->CreateMigrationInfo( &nc_4, nodeid_suid_map ) );

            CHECK( p_mi_4->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_4 = p_mi_4->GetReachableNodes();
            CHECK_EQUAL( 3, reachable_nodes_4.size() );
            CHECK_EQUAL( 1, reachable_nodes_4[ 0].data );
            CHECK_EQUAL( 2, reachable_nodes_4[ 1].data );
            CHECK_EQUAL( 3, reachable_nodes_4[ 2].data );

            const std::vector<MigrationType::Enum>& mig_type_list_4 = p_mi_4->GetMigrationTypes();
            CHECK_EQUAL( 3, mig_type_list_4.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_4[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_4[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list_4[ 2] );

            // ================
            // === FROM NODE 2
            // ================
            m_RandomFake.SetUL( 2576980377 ); // 0.6

            IndividualHumanContextFake traveler( nullptr, &nc_2, nullptr, nullptr );

            // ------------------------------------------------------------------
            // --- Test that male < 15 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 10.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
            float trip_time = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.00001 );

            // ------------------------------------------------------------------
            // --- Test that male = 30 will migrate to node 4
            // ------------------------------------------------------------------
            traveler.SetAge( 30.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 4, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 );

            // ------------------------------------------------------------------
            // --- Test that male = 60 will migrate to node 3
            // ------------------------------------------------------------------
            traveler.SetAge( 60.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 3, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 ); //trip_time is the same because random number is the same

            // ------------------------------------------------------------------
            // --- Test that male = 90 will migrate to node 1
            // ------------------------------------------------------------------
            traveler.SetAge( 90.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            m_RandomFake.SetUL( 0 ); // 0.0 - need to change so we get the first in the list

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 1, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 15.9424, trip_time, 0.0001 ); //trip_time is different due to new random number

            // ------------------------------------------------------------------
            // --- Test that male = 120 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 120.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.0001 );

            // ======================================================================================
            // === Switch to female and verify that we are working with different data than for MALE
            // ======================================================================================
            m_RandomFake.SetUL( 2576980377 ); // 0.6 - reset 
            // ------------------------------------------------------------------
            // --- Test that female < 15 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 10.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.00001 );

            // ------------------------------------------------------------------
            // --- Test that female = 30 will migrate to node 3 instead of 4
            // ------------------------------------------------------------------
            traveler.SetAge( 30.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 3, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 ); //trip_time is the same because random number is the same

            // ------------------------------------------------------------------
            // --- Test that female = 60 will migrate to node 3
            // ------------------------------------------------------------------
            traveler.SetAge( 60.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 3, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 0.5108, trip_time, 0.0001 ); //trip_time is the same because random number is the same

            // ------------------------------------------------------------------
            // --- Test that female = 90 will migrate to node 1
            // ------------------------------------------------------------------
            traveler.SetAge( 90.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            m_RandomFake.SetUL( 0 ); // 0.0 - 

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 1, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 15.9424, trip_time, 0.0001 ); //trip_time is different due to new random number

            // ------------------------------------------------------------------
            // --- Test that female = 120 will NOT migrate
            // ------------------------------------------------------------------
            traveler.SetAge( 120.0*DAYSPERYEAR );
            traveler.SetGender( Gender::FEMALE );

            destination = suids::nil_suid();
            mig_type    = MigrationType::NO_MIGRATION;
            trip_time   = -1.0;

            p_mi_2->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 0, destination.data );
            CHECK_EQUAL( MigrationType::NO_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.0001 );

        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(MigrationFixture, TestFixedRate)
    {
        try
        {
            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= 26 ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }


            std::string idreference = "Household-Scenario-Small" ;
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( EnvPtr->Config,
                                                                                                     idreference, 
                                                                                                     SimType::GENERIC_SIM,
                                                                                                     MigrationStructure::FIXED_RATE_MIGRATION,
                                                                                                     false, 
                                                                                                     10 ) );

            CHECK( p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;
            unique_ptr<IMigrationInfo> p_mi( p_mf->CreateMigrationInfo( &nc_1, nodeid_suid_map ) );

            CHECK( p_mi->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes = p_mi->GetReachableNodes();
            CHECK_EQUAL( 20, reachable_nodes.size() );
            CHECK_EQUAL(  2, reachable_nodes[ 0].data );
            CHECK_EQUAL(  6, reachable_nodes[ 1].data );
            CHECK_EQUAL(  3, reachable_nodes[ 2].data );
            CHECK_EQUAL(  4, reachable_nodes[ 3].data );
            CHECK_EQUAL(  5, reachable_nodes[ 4].data );
            CHECK_EQUAL(  8, reachable_nodes[ 5].data );
            CHECK_EQUAL(  9, reachable_nodes[ 6].data );
            CHECK_EQUAL( 10, reachable_nodes[ 7].data );
            CHECK_EQUAL( 11, reachable_nodes[ 8].data );
            CHECK_EQUAL( 12, reachable_nodes[ 9].data );
            CHECK_EQUAL( 13, reachable_nodes[10].data );
            CHECK_EQUAL( 15, reachable_nodes[11].data );
            CHECK_EQUAL( 17, reachable_nodes[12].data );
            CHECK_EQUAL( 18, reachable_nodes[13].data );
            CHECK_EQUAL( 19, reachable_nodes[14].data );
            CHECK_EQUAL( 20, reachable_nodes[15].data );
            CHECK_EQUAL( 23, reachable_nodes[16].data );
            CHECK_EQUAL( 24, reachable_nodes[17].data );
            CHECK_EQUAL( 25, reachable_nodes[18].data );
            CHECK_EQUAL( 26, reachable_nodes[19].data );

            const std::vector<MigrationType::Enum>& mig_type_list = p_mi->GetMigrationTypes();
            CHECK_EQUAL( 20, mig_type_list.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list[ 1] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 2] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 3] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 4] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 5] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 6] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 7] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 8] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 9] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[10] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[11] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[12] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[13] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[14] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[15] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[16] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[17] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[18] );
            CHECK_EQUAL( MigrationType::SEA_MIGRATION,      mig_type_list[19] );

            // ------------------------------------------------------------------
            // --- Test picking destination that is in the middle of our options
            // ------------------------------------------------------------------
            m_RandomFake.SetUL( 2147486240 ); // 0.5

            IndividualHumanContextFake traveler( nullptr, &nc_1, nullptr, nullptr );
            traveler.SetAge( 20.0*DAYSPERYEAR );
            traveler.SetGender( Gender::MALE );
            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION ;
            float trip_time = -1.0 ;
            p_mi->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 10, destination.data );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type );
            CHECK_CLOSE( 4.33216, trip_time, 0.00001 );

            // -----------------------------------------------
            // --- Test picking our first possible destination
            // -----------------------------------------------
            m_RandomFake.SetUL( 0 ); // 0.0

            destination = suids::nil_suid();
            mig_type = MigrationType::NO_MIGRATION ;
            trip_time = -1.0 ;
            p_mi->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 2, destination.data );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type );
            CHECK_CLOSE( 99.63990, trip_time, 0.00001 );

            // -----------------------------------------------
            // --- Test picking our last possible destination
            // -----------------------------------------------
            m_RandomFake.SetUL( 0xFFFFFFFF ); // 1.0

            destination = suids::nil_suid();
            mig_type = MigrationType::NO_MIGRATION ;
            trip_time = -1.0 ;
            p_mi->PickMigrationStep( &traveler, 1.0, destination, mig_type, trip_time );

            CHECK_EQUAL( 26, destination.data );
            CHECK_EQUAL( MigrationType::SEA_MIGRATION, mig_type );
            CHECK_CLOSE( 0.0, trip_time, 0.00001 );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(MigrationFixture, TestDefault)
    {
        try
        {
            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= 100 ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            std::string idreference = "Default" ;
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( EnvPtr->Config,
                                                                                                     idreference, 
                                                                                                     SimType::GENERIC_SIM,
                                                                                                     MigrationStructure::FIXED_RATE_MIGRATION,
                                                                                                     true, 
                                                                                                     10 ) );

            CHECK( !p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            // ----------
            // --- Node 1 - Upper Left
            // ----------
            INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;
            unique_ptr<IMigrationInfo> p_mi( p_mf->CreateMigrationInfo( &nc_1, nodeid_suid_map ) );

            CHECK( !p_mi->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes = p_mi->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes.size() );
            CHECK_EQUAL( 100, reachable_nodes[ 0].data );
            CHECK_EQUAL(  91, reachable_nodes[ 1].data );
            CHECK_EQUAL(  92, reachable_nodes[ 2].data );
            CHECK_EQUAL(  10, reachable_nodes[ 3].data );
            CHECK_EQUAL(   2, reachable_nodes[ 4].data );
            CHECK_EQUAL(  20, reachable_nodes[ 5].data );
            CHECK_EQUAL(  11, reachable_nodes[ 6].data );
            CHECK_EQUAL(  12, reachable_nodes[ 7].data );

            const std::vector<MigrationType::Enum>& mig_type_list = p_mi->GetMigrationTypes();
            CHECK_EQUAL( 8, mig_type_list.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 2] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 3] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 4] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 5] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 6] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, mig_type_list[ 7] );

            // ----------
            // --- Node 10 - Upper Right
            // ----------
            INodeContextFake nc_10( nodeid_suid_map.left.at(10) ) ;
            unique_ptr<IMigrationInfo> p_mi_10( p_mf->CreateMigrationInfo( &nc_10, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_10 = p_mi_10->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_10.size() );
            CHECK_EQUAL(  99, reachable_nodes_10[ 0].data );
            CHECK_EQUAL( 100, reachable_nodes_10[ 1].data );
            CHECK_EQUAL(  91, reachable_nodes_10[ 2].data );
            CHECK_EQUAL(   9, reachable_nodes_10[ 3].data );
            CHECK_EQUAL(   1, reachable_nodes_10[ 4].data );
            CHECK_EQUAL(  19, reachable_nodes_10[ 5].data );
            CHECK_EQUAL(  20, reachable_nodes_10[ 6].data );
            CHECK_EQUAL(  11, reachable_nodes_10[ 7].data );

            // ----------
            // --- Node 91 - Lower Left
            // ----------
            INodeContextFake nc_91( nodeid_suid_map.left.at(91) ) ;
            unique_ptr<IMigrationInfo> p_mi_91( p_mf->CreateMigrationInfo( &nc_91, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_91 = p_mi_91->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_91.size() );
            CHECK_EQUAL(  90, reachable_nodes_91[ 0].data );
            CHECK_EQUAL(  81, reachable_nodes_91[ 1].data );
            CHECK_EQUAL(  82, reachable_nodes_91[ 2].data );
            CHECK_EQUAL( 100, reachable_nodes_91[ 3].data );
            CHECK_EQUAL(  92, reachable_nodes_91[ 4].data );
            CHECK_EQUAL(  10, reachable_nodes_91[ 5].data );
            CHECK_EQUAL(   1, reachable_nodes_91[ 6].data );
            CHECK_EQUAL(   2, reachable_nodes_91[ 7].data );

            // ----------
            // --- Node 100 - Lower Right
            // ----------
            INodeContextFake nc_100( nodeid_suid_map.left.at(100) ) ;
            unique_ptr<IMigrationInfo> p_mi_100( p_mf->CreateMigrationInfo( &nc_100, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_100 = p_mi_100->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_100.size() );
            CHECK_EQUAL(  89, reachable_nodes_100[ 0].data );
            CHECK_EQUAL(  90, reachable_nodes_100[ 1].data );
            CHECK_EQUAL(  81, reachable_nodes_100[ 2].data );
            CHECK_EQUAL(  99, reachable_nodes_100[ 3].data );
            CHECK_EQUAL(  91, reachable_nodes_100[ 4].data );
            CHECK_EQUAL(   9, reachable_nodes_100[ 5].data );
            CHECK_EQUAL(  10, reachable_nodes_100[ 6].data );
            CHECK_EQUAL(   1, reachable_nodes_100[ 7].data );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(MigrationFixture, TestDefaultSize9)
    {
        try
        {
            int torus_size = 3 ;
            uint32_t num_nodes = torus_size*torus_size;

            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= num_nodes ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            std::string idreference = "Default" ;
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( EnvPtr->Config,
                                                                                                     idreference, 
                                                                                                     SimType::GENERIC_SIM,
                                                                                                     MigrationStructure::FIXED_RATE_MIGRATION,
                                                                                                     true, 
                                                                                                     torus_size ) );

            CHECK( !p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            // ----------
            // --- Node 1 - Upper Left
            // ----------
            INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;
            unique_ptr<IMigrationInfo> p_mi( p_mf->CreateMigrationInfo( &nc_1, nodeid_suid_map ) );

            CHECK( !p_mi->IsHeterogeneityEnabled() );

            const std::vector<suids::suid>& reachable_nodes_1 = p_mi->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_1.size() );
            CHECK_EQUAL( 9, reachable_nodes_1[ 0].data );
            CHECK_EQUAL( 7, reachable_nodes_1[ 1].data );
            CHECK_EQUAL( 8, reachable_nodes_1[ 2].data );
            CHECK_EQUAL( 3, reachable_nodes_1[ 3].data );
            CHECK_EQUAL( 2, reachable_nodes_1[ 4].data );
            CHECK_EQUAL( 6, reachable_nodes_1[ 5].data );
            CHECK_EQUAL( 4, reachable_nodes_1[ 6].data );
            CHECK_EQUAL( 5, reachable_nodes_1[ 7].data );

            // ----------
            // --- Node 3 - Upper Right
            // ----------
            INodeContextFake nc_3( nodeid_suid_map.left.at(3) ) ;
            unique_ptr<IMigrationInfo> p_mi_3( p_mf->CreateMigrationInfo( &nc_3, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_3 = p_mi_3->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_3.size() );
            CHECK_EQUAL( 8, reachable_nodes_3[ 0].data );
            CHECK_EQUAL( 9, reachable_nodes_3[ 1].data );
            CHECK_EQUAL( 7, reachable_nodes_3[ 2].data );
            CHECK_EQUAL( 2, reachable_nodes_3[ 3].data );
            CHECK_EQUAL( 1, reachable_nodes_3[ 4].data );
            CHECK_EQUAL( 5, reachable_nodes_3[ 5].data );
            CHECK_EQUAL( 6, reachable_nodes_3[ 6].data );
            CHECK_EQUAL( 4, reachable_nodes_3[ 7].data );

            // ----------
            // --- Node 7 - Lower Left
            // ----------
            INodeContextFake nc_7( nodeid_suid_map.left.at(7) ) ;
            unique_ptr<IMigrationInfo> p_mi_7( p_mf->CreateMigrationInfo( &nc_7, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_7 = p_mi_7->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_7.size() );
            CHECK_EQUAL( 6, reachable_nodes_7[ 0].data );
            CHECK_EQUAL( 4, reachable_nodes_7[ 1].data );
            CHECK_EQUAL( 5, reachable_nodes_7[ 2].data );
            CHECK_EQUAL( 9, reachable_nodes_7[ 3].data );
            CHECK_EQUAL( 8, reachable_nodes_7[ 4].data );
            CHECK_EQUAL( 3, reachable_nodes_7[ 5].data );
            CHECK_EQUAL( 1, reachable_nodes_7[ 6].data );
            CHECK_EQUAL( 2, reachable_nodes_7[ 7].data );

            // ----------
            // --- Node 9 - Lower right
            // ----------
            INodeContextFake nc_9( nodeid_suid_map.left.at(9) ) ;
            unique_ptr<IMigrationInfo> p_mi_9( p_mf->CreateMigrationInfo( &nc_9, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_9 = p_mi_9->GetReachableNodes();
            CHECK_EQUAL( 8, reachable_nodes_9.size() );
            CHECK_EQUAL( 5, reachable_nodes_9[ 0].data );
            CHECK_EQUAL( 6, reachable_nodes_9[ 1].data );
            CHECK_EQUAL( 4, reachable_nodes_9[ 2].data );
            CHECK_EQUAL( 8, reachable_nodes_9[ 3].data );
            CHECK_EQUAL( 7, reachable_nodes_9[ 4].data );
            CHECK_EQUAL( 2, reachable_nodes_9[ 5].data );
            CHECK_EQUAL( 3, reachable_nodes_9[ 6].data );
            CHECK_EQUAL( 1, reachable_nodes_9[ 7].data );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(MigrationFixture, TestVectorMigrationInfo)
    {
        try
        {
            // --------------------
            // --- Initialize test
            // --------------------
            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= 26 ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            std::string idreference = "Household-Scenario-Small" ;
            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( EnvPtr->Config,
                                                                                                     idreference, 
                                                                                                     SimType::VECTOR_SIM,
                                                                                                     MigrationStructure::FIXED_RATE_MIGRATION,
                                                                                                     false, 
                                                                                                     10 ) );

            CHECK( p_mf->IsAtLeastOneTypeConfiguredForIndividuals() );

            IMigrationInfoFactoryVector* p_mfv = dynamic_cast<IMigrationInfoFactoryVector*>(p_mf.get());
            CHECK( p_mfv != nullptr );

            INodeContextFake nc_1( nodeid_suid_map.left.at(1) ) ;
            unique_ptr<IMigrationInfoVector> p_mi( p_mfv->CreateMigrationInfoVector( &nc_1, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes = p_mi->GetReachableNodes();
            CHECK_EQUAL( 24, reachable_nodes.size() );
            CHECK_EQUAL(  2, reachable_nodes[ 0].data );
            CHECK_EQUAL(  6, reachable_nodes[ 1].data );
            CHECK_EQUAL(  7, reachable_nodes[ 2].data );
            CHECK_EQUAL(  3, reachable_nodes[ 3].data );
            CHECK_EQUAL(  4, reachable_nodes[ 4].data );
            CHECK_EQUAL(  5, reachable_nodes[ 5].data );
            CHECK_EQUAL(  8, reachable_nodes[ 6].data );
            CHECK_EQUAL(  9, reachable_nodes[ 7].data );
            CHECK_EQUAL( 10, reachable_nodes[ 8].data );
            CHECK_EQUAL( 11, reachable_nodes[ 9].data );
            CHECK_EQUAL( 12, reachable_nodes[10].data );
            CHECK_EQUAL( 13, reachable_nodes[11].data );
            CHECK_EQUAL( 14, reachable_nodes[12].data );
            CHECK_EQUAL( 15, reachable_nodes[13].data );
            CHECK_EQUAL( 16, reachable_nodes[14].data );
            CHECK_EQUAL( 17, reachable_nodes[15].data );
            CHECK_EQUAL( 18, reachable_nodes[16].data );
            CHECK_EQUAL( 19, reachable_nodes[17].data );
            CHECK_EQUAL( 20, reachable_nodes[18].data );
            CHECK_EQUAL( 21, reachable_nodes[19].data );
            CHECK_EQUAL( 22, reachable_nodes[20].data );
            CHECK_EQUAL( 23, reachable_nodes[21].data );
            CHECK_EQUAL( 24, reachable_nodes[22].data );
            CHECK_EQUAL( 25, reachable_nodes[23].data );

            const std::vector<MigrationType::Enum>& mig_type_list = p_mi->GetMigrationTypes();
            CHECK_EQUAL( 24, mig_type_list.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list[ 2] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 3] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 4] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 5] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 6] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 7] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 8] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[ 9] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[10] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[11] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[12] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[13] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[14] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[15] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[16] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[17] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[18] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[19] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[20] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[21] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[22] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list[23] );

            INodeContextFake nc_9( nodeid_suid_map.left.at(9) ) ;
            unique_ptr<IMigrationInfo> p_mi_9( p_mfv->CreateMigrationInfoVector( &nc_9, nodeid_suid_map ) );

            const std::vector<suids::suid>& reachable_nodes_9 = p_mi_9->GetReachableNodes();
            CHECK_EQUAL( 24, reachable_nodes_9.size() );
            CHECK_EQUAL(  3, reachable_nodes_9[ 0].data );
            CHECK_EQUAL(  4, reachable_nodes_9[ 1].data );
            CHECK_EQUAL(  5, reachable_nodes_9[ 2].data );
            CHECK_EQUAL(  8, reachable_nodes_9[ 3].data );
            CHECK_EQUAL( 10, reachable_nodes_9[ 4].data );
            CHECK_EQUAL( 13, reachable_nodes_9[ 5].data );
            CHECK_EQUAL( 14, reachable_nodes_9[ 6].data );
            CHECK_EQUAL( 15, reachable_nodes_9[ 7].data );
            CHECK_EQUAL(  1, reachable_nodes_9[ 8].data );
            CHECK_EQUAL(  2, reachable_nodes_9[ 9].data );
            CHECK_EQUAL(  6, reachable_nodes_9[10].data );
            CHECK_EQUAL(  7, reachable_nodes_9[11].data );
            CHECK_EQUAL( 11, reachable_nodes_9[12].data );
            CHECK_EQUAL( 12, reachable_nodes_9[13].data );
            CHECK_EQUAL( 16, reachable_nodes_9[14].data );
            CHECK_EQUAL( 17, reachable_nodes_9[15].data );
            CHECK_EQUAL( 18, reachable_nodes_9[16].data );
            CHECK_EQUAL( 19, reachable_nodes_9[17].data );
            CHECK_EQUAL( 20, reachable_nodes_9[18].data );
            CHECK_EQUAL( 21, reachable_nodes_9[19].data );
            CHECK_EQUAL( 22, reachable_nodes_9[20].data );
            CHECK_EQUAL( 23, reachable_nodes_9[21].data );
            CHECK_EQUAL( 24, reachable_nodes_9[22].data );
            CHECK_EQUAL( 25, reachable_nodes_9[23].data );

            const std::vector<MigrationType::Enum>& mig_type_list_9 = p_mi_9->GetMigrationTypes();
            CHECK_EQUAL( 24, mig_type_list_9.size() );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 0] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 1] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 2] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 3] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 4] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 5] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 6] );
            CHECK_EQUAL( MigrationType::LOCAL_MIGRATION,    mig_type_list_9[ 7] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[ 8] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[ 9] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[10] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[11] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[12] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[13] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[14] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[15] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[16] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[17] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[18] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[19] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[20] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[21] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[22] );
            CHECK_EQUAL( MigrationType::REGIONAL_MIGRATION, mig_type_list_9[23] );

            INodeContextFake nc_26( nodeid_suid_map.left.at(26) ) ;
            unique_ptr<IMigrationInfo> p_mi_26( p_mfv->CreateMigrationInfoVector( &nc_26, nodeid_suid_map ) );
            CHECK( p_mi_26->GetReachableNodes().size() == 0 );
        }
        catch( DetailedException& re )
        {
            printf("%s\n",re.GetMsg());
            CHECK( false );
        }
    }

    void TestHelper_FactoryConfigureException( int lineNumber, 
                                               const std::string& rFilename, 
                                               const std::string& rIdReference,
                                               uint32_t numNodes,
                                               ExternalNodeId_t nodeId,
                                               const std::string& rExpMsg )
    {
        try
        {
            unique_ptr<Configuration> p_config_file( Environment::LoadConfigurationFile( rFilename.c_str() ) );

            unique_ptr<Configuration> p_config( Environment::CopyFromElement( (*p_config_file)["parameters"] ) );

            nodeid_suid_map_t nodeid_suid_map;
            for( uint32_t node_id = 1 ; node_id <= numNodes ; node_id++ )
            {
                suids::suid node_suid ;
                node_suid.data = node_id ;
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));
            }

            unique_ptr<IMigrationInfoFactory> p_mf( MigrationFactory::ConstructMigrationInfoFactory( p_config.get(),
                                                                                                     rIdReference,
                                                                                                     SimType::GENERIC_SIM,
                                                                                                     MigrationStructure::FIXED_RATE_MIGRATION,
                                                                                                     false,
                                                                                                     10 ) );

            suids::suid node_suid;
            node_suid.data = nodeId;
            INodeContextFake nc( node_suid ) ;
            unique_ptr<IMigrationInfo> p_mi( p_mf->CreateMigrationInfo( &nc, nodeid_suid_map ) );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            bool passed = msg.find( rExpMsg ) != string::npos ;
            if( !passed )
            {
                PrintDebug( msg );
            }
            CHECK_LN( passed, lineNumber );
        }
    }


    TEST_FIXTURE(MigrationFixture, TestNoLocalFilename)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestNoLocalFilename_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'Enable_Local_Migration' with value 1 is incompatible with variable or parameter 'Local_Migration_Filename' with value <empty>." );
    }

    TEST_FIXTURE(MigrationFixture, TestNoAirFilename)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestNoAirFilename_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'Enable_Air_Migration' with value 1 is incompatible with variable or parameter 'Air_Migration_Filename' with value <empty>." );
    }

    TEST_FIXTURE(MigrationFixture, TestNoRegionalFilename)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestNoRegionalFilename_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'Enable_Regional_Migration' with value 1 is incompatible with variable or parameter 'Regional_Migration_Filename' with value <empty>." );
    }

    TEST_FIXTURE(MigrationFixture, TestNoSeaFilename)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestNoSeaFilename_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'Enable_Sea_Migration' with value 1 is incompatible with variable or parameter 'Sea_Migration_Filename' with value <empty>." );
    }

    TEST_FIXTURE(MigrationFixture, TestNoMetadataFile)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestNoMetadataFile_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Could not find file testdata/MigrationTest/5x5_Households_Local_MigrationXXX.bin.json" );
    }

    TEST_FIXTURE(MigrationFixture, TestBadIdReference)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/config.json", 
            "BadIdReference", 
            26,
            1,
            "Variable or parameter 'idreference' with value BadIdReference is incompatible with variable or parameter 'testdata/MigrationTest/5x5_Households_Local_Migration.bin.json[Metadata][IdReference]' with value Household-Scenario-Small. " );
    }

    TEST_FIXTURE(MigrationFixture, TestDatavalueCountTooSmall)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestDatavalueCountTooSmall_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable testdata/MigrationTest/TestDatavalueCountTooSmall.bin.json[Metadata][DatavalueCount] had value 0 which was inconsistent with range limit 0" );
    }

    TEST_FIXTURE(MigrationFixture, TestDatavalueCountTooBig)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestDatavalueCountTooBig_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable testdata/MigrationTest/TestDatavalueCountTooBig.bin.json[Metadata][DatavalueCount] had value 999 which was inconsistent with range limit 100" );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidMigrationTypeA)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidMigrationTypeA_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'm_MigrationType' with value LOCAL_MIGRATION is incompatible with variable or parameter 'testdata/MigrationTest/TestInvalidMigrationTypeA.bin.json[Metadata][MigrationType]' with value SEA_MIGRATION. ");
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidMigrationTypeB)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidMigrationTypeB_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestInvalidMigrationTypeB.bin.json[Metadata][MigrationType] = 'XXX' is not a valid MigrationType.  Valid values are: 'NO_MIGRATION', 'LOCAL_MIGRATION', 'AIR_MIGRATION', 'REGIONAL_MIGRATION', 'SEA_MIGRATION'" );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidGenderDataType)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidGenderDataType_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestInvalidGenderDataType.bin.json[Metadata][GenderDataType] = 'XXX' is not a valid GenderDataType.  Valid values are: 'SAME_FOR_BOTH_GENDERS', 'ONE_FOR_EACH_GENDER'" );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidAgesYearsNotArray)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidAgesYearsNotArray_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestInvalidAgesYearsNotArray.bin.json[Metadata][AgesYears] must be an array of ages in years between 0 and 125 and must be in increasing order." );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidAgesYearsLessThanZero)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidAgesYearsLessThanZero_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestInvalidAgesYearsLessThanZero.bin.json[Metadata][AgesYears][0] = -1.  testdata/MigrationTest/TestInvalidAgesYearsLessThanZero.bin.json[Metadata][AgesYears] must be an array of ages in years between 0 and 125 and must be in increasing order." );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidAgesYearsGreaterThanMax)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidAgesYearsGreaterThanMax_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestInvalidAgesYearsGreaterThanMax.bin.json[Metadata][AgesYears][1] = 999.  testdata/MigrationTest/TestInvalidAgesYearsGreaterThanMax.bin.json[Metadata][AgesYears] must be an array of ages in years between 0 and 125 and must be in increasing order.");
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidAgesYearsOrder)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidAgesYearsOrder_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestInvalidAgesYearsOrder.bin.json[Metadata][AgesYears] must be an array of ages in years between 0 and 125 and must be in increasing order.");
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidInterpolationType)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidInterpolationType_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestInvalidInterpolationType.bin.json[Metadata][InterpolationType] = 'XXX' is not a valid InterpolationType.  Valid values are: 'LINEAR_INTERPOLATION', 'PIECEWISE_CONSTANT'" );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidOffset)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidOffset_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Variable or parameter 'offsets_str.length() / 16' with value 26 is incompatible with variable or parameter 'num_nodes' with value 999. " );
    }

    TEST_FIXTURE(MigrationFixture, TestMetadataBadJsonA)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestMetadataBadJsonA_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestMetadataBadJsonA.bin.json: Failed to parse incoming text. Name of an object member must be a string at character=312 / line number=10" );
    }

    TEST_FIXTURE(MigrationFixture, TestMetadataBadJsonB)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestMetadataBadJsonB_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestMetadataBadJsonB.bin.json: The 'InterpolationType' element is not a 'String'." );
    }

    TEST_FIXTURE(MigrationFixture, TestMetadataBadJsonC)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestMetadataBadJsonC_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "testdata/MigrationTest/TestMetadataBadJsonC.bin.json: The 'Metadata' element does not contain an element with name 'NodeCount'." );
    }

    TEST_FIXTURE(MigrationFixture, TestLocalMigrationFileNotFound)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestLocalMigrationFileNotFound_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "Could not find file testdata/MigrationTest/TestLocalMigrationFileNotFound.bin" );
    }

    TEST_FIXTURE(MigrationFixture, TestWrongSize)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestWrongSize_config.json", 
            "Household-Scenario-Small", 
            26,
            1,
            "I/O error while reading/writing. File name =  testdata/MigrationTest/TestWrongSize.bin.  Detected wrong size for migration data file.  Expected 2400 bytes, read 2496 bytes" );
    }

    TEST_FIXTURE(MigrationFixture, TestNodeNotFound)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/config.json", 
            "Household-Scenario-Small", 
            26,
            999,
            "Variable or parameter 'rNodeIdSuidMap.right.count(node_suid)' with value 0 is incompatible with variable or parameter 'node_suid' with value 999.");
    }

    TEST_FIXTURE(MigrationFixture, TestNodesInFileNotInScenario)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/config.json", 
            "Household-Scenario-Small", 
            5,
            1,
            "NodeId, 6, found in 5x5_Households_Local_Migration.bin, is not a node in the simulation." );
    }

    TEST_FIXTURE(MigrationFixture, TestInvalidAgeDataSection)
    {
        TestHelper_FactoryConfigureException( 
            __LINE__, 
            "testdata/MigrationTest/TestInvalidAgeDataSection_config.json", 
            "ABC", 
            4,
            2,
            "In file 'TestInvalidAgeDataSection.bin', the 'To' Node IDs are not the same for the Age Data sections for fromNodeId = 2" );
    }

    TEST_FIXTURE( MigrationFixture, TestInvalidOffsetValues )
    {
        TestHelper_FactoryConfigureException(
            __LINE__,
            "testdata/MigrationTest/TestInvalidOffsetValues_config.json",
            "Household-Scenario-Small",
            26,
            26,
            "I/O error while reading/writing. File name =  TestInvalidOffsetValues.bin.  \nInvalid 'NodeOffsets' in testdata/MigrationTest/TestInvalidOffsetValues.bin.json.\nNode ID=26 has an offset of 0xbadbeef but the '.bin' file size is expected to be 2496(0x9c0)." );
    }
}
