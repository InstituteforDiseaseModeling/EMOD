/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "AssortivityFactory.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "RandomFake.h"
#include "Node.h"
#include "SimulationConfig.h"
#include "HIVEnums.h"
#include "Properties.h"
#include "common.h"

using namespace std; 
using namespace Kernel; 

// maybe these shouldn't be protected in Simulation.h
typedef boost::bimap<ExternalNodeId_t, suids::suid> nodeid_suid_map_t;
typedef nodeid_suid_map_t::value_type nodeid_suid_pair;


SUITE(AssortivityTest)
{
    static int m_NextId = 1 ;

    struct AssortivityFixture
    {
        INodeContextFake        m_NC ;
        INodeEventContextFake   m_NEC ;
        vector< IndividualHumanInterventionsContextFake* > m_hic_list ;
        vector< IndividualHumanContextFake*              > m_human_list ;
        SimulationConfig* m_pSimulationConfig ;

        AssortivityFixture()
            : m_NC()
            , m_NEC()
            , m_hic_list()
            , m_human_list()
            , m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            m_pSimulationConfig->sim_type = SimType::HIV_SIM ;
            Environment::setSimulationConfig( m_pSimulationConfig );
            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();

            std::map<std::string,float> ip_values ;
            ip_values.insert( std::make_pair( "HUMAN",    0.2f ) );
            ip_values.insert( std::make_pair( "VULCAN",   0.2f ) );
            ip_values.insert( std::make_pair( "KLINGON",  0.2f ) );
            ip_values.insert( std::make_pair( "ANDORIAN", 0.2f ) );
            ip_values.insert( std::make_pair( "ROMULAN",  0.2f ) );

            IPFactory::GetInstance()->AddIP( 1, "Race", ip_values );
        }

        ~AssortivityFixture()
        {
            for( auto hic : m_hic_list )
            {
                delete hic ;
            }
            m_hic_list.clear();

            for( auto human : m_human_list )
            {
                delete human ;
            }
            m_human_list.clear();
            IPFactory::DeleteFactory();
            Environment::Finalize();
        }

        IIndividualHumanSTI* CreateHuman( int gender, 
                                          float ageDays, 
                                          bool hasSTI, 
                                          bool hasHIV, 
                                          bool hasCoSTI, 
                                          bool hasTestedPositiveForHIV,
                                          ReceivedTestResultsType::Enum receiveResults = ReceivedTestResultsType::UNKNOWN,
                                          const std::string& rPropertyName = std::string(), 
                                          const std::string& rPropertyValue = std::string() )
        {
            IndividualHumanInterventionsContextFake* p_hic = new IndividualHumanInterventionsContextFake();
            IndividualHumanContextFake* p_human = new IndividualHumanContextFake( p_hic, &m_NC, &m_NEC, nullptr );

            if( hasTestedPositiveForHIV )
            {
                release_assert( hasHIV );
            }
            p_human->SetId( m_NextId++ );
            p_human->SetGender( gender );
            p_human->SetAge( ageDays );
            p_human->SetHasSTI( hasSTI );
            p_human->SetHasHIV( hasHIV );
            p_hic->OnTestForHIV( hasTestedPositiveForHIV );
            //p_human->GetProperties()->operator[]( rPropertyName ) = rPropertyValue ;
            if( !rPropertyName.empty()  && !rPropertyValue.empty() )
            {
                p_human->GetProperties()->Add( IPKeyValue( rPropertyName, rPropertyValue ) ) ;
            }

            if( hasCoSTI )
                p_human->SetStiCoInfectionState();
            else
                p_human->ClearStiCoInfectionState();

            if( receiveResults == ReceivedTestResultsType::POSITIVE )
                p_hic->OnReceivedTestResultForHIV(true);
            else if( receiveResults == ReceivedTestResultsType::NEGATIVE )
                p_hic->OnReceivedTestResultForHIV(false);

            m_hic_list.push_back( p_hic );
            m_human_list.push_back( p_human );

            return (IIndividualHumanSTI*)p_human ;
        }
    };


    TEST_FIXTURE(AssortivityFixture, TestNoGroup)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/AssortivityTest/NoGroup.json" ) );

        RandomFake fake_ran ;
        unique_ptr<IAssortivity> p_assort( AssortivityFactory::CreateAssortivity( RelationshipType::TRANSITORY, &fake_ran ) );

        CHECK( p_assort->Configure( p_config.get() ) );

        // -----------------------------------------------------------------
        // --- Test that we get a nullptr if there are no women in the list
        // -----------------------------------------------------------------
        IIndividualHumanSTI* p_m  = CreateHuman( Gender::MALE, 25.0f*365.0f, false, false, false, false );
        list<IIndividualHumanSTI*> female_list ;


        IIndividualHumanSTI* p_female_match = p_assort->SelectPartner( p_m, female_list );

        CHECK( p_female_match == nullptr );

        // ----------------------------------------------------------------------------------
        // --- Test that if there are two women in the list we get the first one in the list
        // ----------------------------------------------------------------------------------
        IIndividualHumanSTI* p_f1 = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, false, false );
        IIndividualHumanSTI* p_f2 = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, false, false );
        female_list.push_back( p_f1 );
        female_list.push_back( p_f2 );

        p_female_match = p_assort->SelectPartner( p_m, female_list );

        CHECK( p_f1 == p_female_match );
    }

    TEST_FIXTURE(AssortivityFixture, TestHivStatus)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/AssortivityTest/HivInfectionStatus.json" ) );

        RandomFake fake_ran ;
        unique_ptr<IAssortivity> p_assort( AssortivityFactory::CreateAssortivity( RelationshipType::TRANSITORY, &fake_ran ) );

        CHECK( p_assort->Configure( p_config.get() ) );

        // -----------------------------------------------------------------
        // --- Test that we get a nullptr if there are no women in the list
        // -----------------------------------------------------------------
        IIndividualHumanSTI* p_m_h   = CreateHuman( Gender::MALE, 25.0f*365.0f, false, true,  false, false );
        IIndividualHumanSTI* p_m_nh  = CreateHuman( Gender::MALE, 25.0f*365.0f, false, false, false, false );
        list<IIndividualHumanSTI*> female_list ;

        IIndividualHumanSTI* p_female_match = p_assort->SelectPartner( p_m_nh, female_list );

        CHECK( p_female_match == nullptr );

        // ----------------------------------------------------------------------------------
        // --- The matrix in the data file prioritizes the relationships as follows:
        // ---  M-H + F-H, M-NH + F-NH, M-H + F-NH, M-NH + F-H
        // ----------------------------------------------------------------------------------
        IIndividualHumanSTI* p_f_h  = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, true,  false, false );
        IIndividualHumanSTI* p_f_nh = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, false, false );
        female_list.push_back( p_f_nh );
        female_list.push_back( p_f_h  );

        // -------------------------------------------------------------
        // --- Test that if the current year is below the start year, 
        // --- we are in NO_GROUP and get the first person in the list.
        // -------------------------------------------------------------
        IdmDateTime date_time_1980( 1980.0f * 365.0f ) ;
        p_assort->Update( date_time_1980, 30.0f );

        IIndividualHumanSTI* p_female_match_1 = p_assort->SelectPartner( p_m_h,  female_list );
        IIndividualHumanSTI* p_female_match_2 = p_assort->SelectPartner( p_m_nh, female_list );

        CHECK( p_f_nh == p_female_match_1 );
        CHECK( p_f_nh == p_female_match_2 );

        // -------------------------------------------------------------
        // --- Test that if the current year is after the start year, 
        // --- the partners are selected based on the priorities.
        // -------------------------------------------------------------
        IdmDateTime date_time_2000( 2000.0f * 365.0f ) ;
        p_assort->Update( date_time_2000, 30.0f );

        fake_ran.SetUL( 2147486240 ); // 0.5

        IIndividualHumanSTI* p_female_match_3 = p_assort->SelectPartner( p_m_h,  female_list );
        IIndividualHumanSTI* p_female_match_4 = p_assort->SelectPartner( p_m_nh, female_list );

        CHECK( p_f_h  == p_female_match_3 );
        CHECK( p_f_nh == p_female_match_4 );
    }

    TEST_FIXTURE(AssortivityFixture, TestHivTestedPositiveStatus)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/AssortivityTest/HivTestedPositiveStatus.json" ) );

        RandomFake fake_ran ;
        unique_ptr<IAssortivity> p_assort( AssortivityFactory::CreateAssortivity( RelationshipType::TRANSITORY, &fake_ran ) );

        CHECK( p_assort->Configure( p_config.get() ) );

        // -----------------------------------------------------------------
        // --- Test that we get a nullptr if there are no women in the list
        // -----------------------------------------------------------------
        IIndividualHumanSTI* p_m_h_tp   = CreateHuman( Gender::MALE, 25.0f*365.0f, false, true, false, true  );
        IIndividualHumanSTI* p_m_h_ntp  = CreateHuman( Gender::MALE, 25.0f*365.0f, false, true, false, false );
        list<IIndividualHumanSTI*> female_list ;

        IIndividualHumanSTI* p_female_match = p_assort->SelectPartner( p_m_h_tp, female_list );

        CHECK( p_female_match == nullptr );

        // ----------------------------------------------------------------------------------
        // --- The matrix in the data file prioritizes the relationships as follows:
        // ---  M-H + F-H, M-NH + F-NH, M-H + F-NH, M-NH + F-H
        // ----------------------------------------------------------------------------------
        IIndividualHumanSTI* p_f_h_tp  = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, true, false, true  );
        IIndividualHumanSTI* p_f_h_ntp = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, true, false, false );
        female_list.push_back( p_f_h_ntp );
        female_list.push_back( p_f_h_tp  );

        // -------------------------------------------------------------
        // --- Test that if the current year is below the start year, 
        // --- we are in NO_GROUP and get the first person in the list.
        // -------------------------------------------------------------
        IdmDateTime date_time_1980( 1980.0f * 365.0f ) ;
        p_assort->Update( date_time_1980, 30.0f );

        IIndividualHumanSTI* p_female_match_1 = p_assort->SelectPartner( p_m_h_tp,  female_list );
        IIndividualHumanSTI* p_female_match_2 = p_assort->SelectPartner( p_m_h_ntp, female_list );

        CHECK( p_f_h_ntp == p_female_match_1 );
        CHECK( p_f_h_ntp == p_female_match_2 );

        // -------------------------------------------------------------
        // --- Test that if the current year is after the start year, 
        // --- the partners are selected based on the priorities.
        // -------------------------------------------------------------
        IdmDateTime date_time_2000( 2000.0f * 365.0f ) ;
        p_assort->Update( date_time_2000, 30.0f );

        fake_ran.SetUL( 2147486240 ); // 0.5

        IIndividualHumanSTI* p_female_match_3 = p_assort->SelectPartner( p_m_h_tp,  female_list );
        IIndividualHumanSTI* p_female_match_4 = p_assort->SelectPartner( p_m_h_ntp, female_list );

        CHECK( p_f_h_tp  == p_female_match_3 );
        CHECK( p_f_h_ntp == p_female_match_4 );
    }

    TEST_FIXTURE(AssortivityFixture, TestHivReceivedResultStatus)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/AssortivityTest/HivReceivedResultStatus.json" ) );

        RandomFake fake_ran ;
        unique_ptr<IAssortivity> p_assort( AssortivityFactory::CreateAssortivity( RelationshipType::TRANSITORY, &fake_ran ) );

        CHECK( p_assort->Configure( p_config.get() ) );

        // -----------------------------------------------------------------
        // --- Test that we get a nullptr if there are no women in the list
        // -----------------------------------------------------------------
        IIndividualHumanSTI* p_m_h_unk = CreateHuman( Gender::MALE, 25.0f*365.0f, false, true, false, true,  ReceivedTestResultsType::UNKNOWN  );
        IIndividualHumanSTI* p_m_h_tp  = CreateHuman( Gender::MALE, 25.0f*365.0f, false, true, false, true,  ReceivedTestResultsType::POSITIVE );
        IIndividualHumanSTI* p_m_h_ntp = CreateHuman( Gender::MALE, 25.0f*365.0f, false, true, false, false, ReceivedTestResultsType::NEGATIVE );

        list<IIndividualHumanSTI*> female_list ;

        IIndividualHumanSTI* p_female_match = p_assort->SelectPartner( p_m_h_tp, female_list );

        CHECK( p_female_match == nullptr );

        // ----------------------------------------------------------------------------------
        // --- The matrix in the data file should be similar to:
        // ---    [ 0.5, 0.1, 0.4 ],
        // ---    [ 0.1, 0.8, 0.1 ],
        // ---    [ 0.1, 0.1, 0.8 ]
        // ----------------------------------------------------------------------------------
        IIndividualHumanSTI* p_f_h_unk = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, true, false, true, ReceivedTestResultsType::UNKNOWN  );
        IIndividualHumanSTI* p_f_h_tp  = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, true, false, true, ReceivedTestResultsType::POSITIVE );
        IIndividualHumanSTI* p_f_h_ntp = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, true, false, false,ReceivedTestResultsType::NEGATIVE );
        female_list.push_back( p_f_h_ntp );
        female_list.push_back( p_f_h_tp  );
        female_list.push_back( p_f_h_unk );

        // -------------------------------------------------------------
        // --- Test that if the current year is below the start year, 
        // --- we are in NO_GROUP and get the first person in the list.
        // -------------------------------------------------------------
        IdmDateTime date_time_1980( 1980.0f * 365.0f ) ;
        p_assort->Update( date_time_1980, 30.0f );

        IIndividualHumanSTI* p_female_match_1 = p_assort->SelectPartner( p_m_h_unk, female_list );
        IIndividualHumanSTI* p_female_match_2 = p_assort->SelectPartner( p_m_h_tp,  female_list );
        IIndividualHumanSTI* p_female_match_3 = p_assort->SelectPartner( p_m_h_ntp, female_list );

        CHECK( p_f_h_ntp == p_female_match_1 );
        CHECK( p_f_h_ntp == p_female_match_2 );
        CHECK( p_f_h_ntp == p_female_match_3 );

        // -------------------------------------------------------------
        // --- Test that if the current year is after the start year, 
        // --- the partners are selected based on the priorities.
        // -------------------------------------------------------------
        IdmDateTime date_time_2000( 2000.0f * 365.0f ) ;
        p_assort->Update( date_time_2000, 30.0f );

        fake_ran.SetUL( 2147486240 ); // 0.5

        IIndividualHumanSTI* p_female_match_4 = p_assort->SelectPartner( p_m_h_unk, female_list );
        IIndividualHumanSTI* p_female_match_5 = p_assort->SelectPartner( p_m_h_tp,  female_list );
        IIndividualHumanSTI* p_female_match_6 = p_assort->SelectPartner( p_m_h_ntp, female_list );

        CHECK( p_f_h_unk == p_female_match_4 );
        CHECK( p_f_h_tp  == p_female_match_5 );
        CHECK( p_f_h_ntp == p_female_match_6 );
    }

    TEST_FIXTURE(AssortivityFixture, TestStiStatus)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        m_pSimulationConfig->sim_type = SimType::STI_SIM ;

        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/AssortivityTest/StiInfectionStatus.json" ) );

        RandomFake fake_ran ;
        unique_ptr<IAssortivity> p_assort( AssortivityFactory::CreateAssortivity( RelationshipType::TRANSITORY, &fake_ran ) );

        CHECK( p_assort->Configure( p_config.get() ) );

        // -----------------------------------------------------------------
        // --- Test that we get a nullptr if there are no women in the list
        // -----------------------------------------------------------------
        IIndividualHumanSTI* p_m_s   = CreateHuman( Gender::MALE, 25.0f*365.0f, true,  false, false, false );
        IIndividualHumanSTI* p_m_ns  = CreateHuman( Gender::MALE, 25.0f*365.0f, false, false, false, false );
        list<IIndividualHumanSTI*> female_list ;

        IIndividualHumanSTI* p_female_match = p_assort->SelectPartner( p_m_ns, female_list );

        CHECK( p_female_match == nullptr );

        // ----------------------------------------------------------------------------------
        // --- The matrix in the data file prioritizes the relationships as follows:
        // ---  M-NS + F-NS, M-S + F-S, M-NS + F-S, M-S + F-NS
        // ----------------------------------------------------------------------------------
        IIndividualHumanSTI* p_f_s  = CreateHuman( Gender::FEMALE, 25.0f*365.0f, true,  false, false, false );
        IIndividualHumanSTI* p_f_ns = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, false, false );
        female_list.push_back( p_f_ns );
        female_list.push_back( p_f_s  );

        // -------------------------------------------------------------
        // --- Test that if the current year is below the start year, 
        // --- we are in NO_GROUP and get the first person in the list.
        // -------------------------------------------------------------
        IdmDateTime date_time( 1980.0f * 365.0f ) ;
        p_assort->Update( date_time, 30.0f );

        fake_ran.SetUL( 2147486240 ); // 0.5

        IIndividualHumanSTI* p_female_match_1 = p_assort->SelectPartner( p_m_s,  female_list );
        IIndividualHumanSTI* p_female_match_2 = p_assort->SelectPartner( p_m_ns, female_list );

        CHECK( p_f_s  == p_female_match_1 );
        CHECK( p_f_ns == p_female_match_2 );
    }

    TEST_FIXTURE(AssortivityFixture, TestStiCoStatus)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/AssortivityTest/StiCoInfectionStatus.json" ) );

        RandomFake fake_ran ;
        unique_ptr<IAssortivity> p_assort( AssortivityFactory::CreateAssortivity( RelationshipType::TRANSITORY, &fake_ran ) );

        CHECK( p_assort->Configure( p_config.get() ) );

        // -----------------------------------------------------------------
        // --- Test that we get a nullptr if there are no women in the list
        // -----------------------------------------------------------------
        IIndividualHumanSTI* p_m_s   = CreateHuman( Gender::MALE, 25.0f*365.0f, false, false, true,  false );
        IIndividualHumanSTI* p_m_ns  = CreateHuman( Gender::MALE, 25.0f*365.0f, false, false, false, false );
        list<IIndividualHumanSTI*> female_list ;

        IIndividualHumanSTI* p_female_match = p_assort->SelectPartner( p_m_ns, female_list );

        CHECK( p_female_match == nullptr );

        // ----------------------------------------------------------------------------------
        // --- The matrix in the data file prioritizes the relationships as follows:
        // ---  M-NS + F-NS, M-S + F-S, M-NS + F-S, M-S + F-NS
        // ----------------------------------------------------------------------------------
        IIndividualHumanSTI* p_f_s  = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, true,  false );
        IIndividualHumanSTI* p_f_ns = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, false, false );
        female_list.push_back( p_f_ns );
        female_list.push_back( p_f_s  );

        // -------------------------------------------------------------
        // --- Test that if the current year is below the start year, 
        // --- we are in NO_GROUP and get the first person in the list.
        // -------------------------------------------------------------
        IdmDateTime date_time_1980( 1980.0f * 365.0f ) ;
        p_assort->Update( date_time_1980, 30.0f );

        IIndividualHumanSTI* p_female_match_1 = p_assort->SelectPartner( p_m_s,  female_list );
        IIndividualHumanSTI* p_female_match_2 = p_assort->SelectPartner( p_m_ns, female_list );

        CHECK( p_f_ns == p_female_match_1 );
        CHECK( p_f_ns == p_female_match_2 );

        // -------------------------------------------------------------
        // --- Test that if the current year is after the start year, 
        // --- the partners are selected based on the priorities.
        // -------------------------------------------------------------
        IdmDateTime date_time_2000( 2000.0f * 365.0f ) ;
        p_assort->Update( date_time_2000, 30.0f );

        fake_ran.SetUL( 2147486240 ); // 0.5

        IIndividualHumanSTI* p_female_match_3 = p_assort->SelectPartner( p_m_s,  female_list );
        IIndividualHumanSTI* p_female_match_4 = p_assort->SelectPartner( p_m_ns, female_list );

        CHECK( p_f_s  == p_female_match_3 );
        CHECK( p_f_ns == p_female_match_4 );
    }

    TEST_FIXTURE(AssortivityFixture, TestIndividualProperty)
    {
        // --------------------
        // --- Initialize test
        // --------------------

        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/AssortivityTest/IndividualProperty.json" ) );

        RandomFake fake_ran ;
        unique_ptr<IAssortivity> p_assort( AssortivityFactory::CreateAssortivity( RelationshipType::TRANSITORY, &fake_ran ) );

        CHECK( p_assort->Configure( p_config.get() ) );

        // -----------------------------------------------------------------
        // --- Test that we get a nullptr if there are no women in the list
        // -----------------------------------------------------------------
        IIndividualHumanSTI* p_m_h = CreateHuman( Gender::MALE, 25.0f*365.0f, false, false, false, false, ReceivedTestResultsType::UNKNOWN, "Race", "HUMAN"    );
        IIndividualHumanSTI* p_m_v = CreateHuman( Gender::MALE, 25.0f*365.0f, false, false, false, false, ReceivedTestResultsType::UNKNOWN, "Race", "VULCAN"   );
        IIndividualHumanSTI* p_m_k = CreateHuman( Gender::MALE, 25.0f*365.0f, false, false, false, false, ReceivedTestResultsType::UNKNOWN, "Race", "KLINGON"  );
        IIndividualHumanSTI* p_m_a = CreateHuman( Gender::MALE, 25.0f*365.0f, false, false, false, false, ReceivedTestResultsType::UNKNOWN, "Race", "ANDORIAN" );
        IIndividualHumanSTI* p_m_r = CreateHuman( Gender::MALE, 25.0f*365.0f, false, false, false, false, ReceivedTestResultsType::UNKNOWN, "Race", "ROMULAN"  );
        list<IIndividualHumanSTI*> female_list ;

        IIndividualHumanSTI* p_female_match = p_assort->SelectPartner( p_m_h, female_list );

        CHECK( p_female_match == nullptr );

        // --------------------------------------
        // --- Fill the list with one of each type
        // --------------------------------------
        IIndividualHumanSTI* p_f_h = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, false, false, ReceivedTestResultsType::UNKNOWN, "Race", "HUMAN"    );
        IIndividualHumanSTI* p_f_v = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, false, false, ReceivedTestResultsType::UNKNOWN, "Race", "VULCAN"   );
        IIndividualHumanSTI* p_f_k = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, false, false, ReceivedTestResultsType::UNKNOWN, "Race", "KLINGON"  );
        IIndividualHumanSTI* p_f_a = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, false, false, ReceivedTestResultsType::UNKNOWN, "Race", "ANDORIAN" );
        IIndividualHumanSTI* p_f_r = CreateHuman( Gender::FEMALE, 25.0f*365.0f, false, false, false, false, ReceivedTestResultsType::UNKNOWN, "Race", "ROMULAN"  );
        female_list.push_back( p_f_h );
        female_list.push_back( p_f_v  );
        female_list.push_back( p_f_k  );
        female_list.push_back( p_f_a  );
        female_list.push_back( p_f_r  );

        // -------------------------------------------------------------
        // --- Test that if the current year is below the start year, 
        // --- we are in NO_GROUP and get the first person in the list.
        // -------------------------------------------------------------
        IdmDateTime date_time_1980( 1980.0f * 365.0f ) ;
        p_assort->Update( date_time_1980, 30.0f );

        IIndividualHumanSTI* p_female_match_1 = p_assort->SelectPartner( p_m_h, female_list );
        IIndividualHumanSTI* p_female_match_2 = p_assort->SelectPartner( p_m_v, female_list );
        IIndividualHumanSTI* p_female_match_3 = p_assort->SelectPartner( p_m_k, female_list );
        IIndividualHumanSTI* p_female_match_4 = p_assort->SelectPartner( p_m_a, female_list );
        IIndividualHumanSTI* p_female_match_5 = p_assort->SelectPartner( p_m_r, female_list );

        CHECK( p_f_h == p_female_match_1 );
        CHECK( p_f_h == p_female_match_2 );
        CHECK( p_f_h == p_female_match_3 );
        CHECK( p_f_h == p_female_match_4 );
        CHECK( p_f_h == p_female_match_5 );

        // -------------------------------------------------------------
        // --- Test that if the current year is after the start year, 
        // --- the partners are selected based on the priorities.
        // -------------------------------------------------------------
        IdmDateTime date_time_2000( 2000.0f * 365.0f ) ;
        p_assort->Update( date_time_2000, 30.0f );

        fake_ran.SetUL( 2147480240 ); // 0.5

        IIndividualHumanSTI* p_female_match_6 = p_assort->SelectPartner( p_m_h, female_list );
        IIndividualHumanSTI* p_female_match_7 = p_assort->SelectPartner( p_m_v, female_list );
        IIndividualHumanSTI* p_female_match_8 = p_assort->SelectPartner( p_m_k, female_list );
        IIndividualHumanSTI* p_female_match_9 = p_assort->SelectPartner( p_m_a, female_list );
        IIndividualHumanSTI* p_female_match_0 = p_assort->SelectPartner( p_m_r, female_list );

        CHECK( p_f_h == p_female_match_6 );
        CHECK( p_f_v == p_female_match_7 );
        CHECK( p_f_k == p_female_match_8 );
        CHECK( p_f_a == p_female_match_9 );
        CHECK( p_f_a == p_female_match_0 ); // the cum of all other others reaches the threshold

        // -------------------------------------------------------
        // --- Test that if our random number draw wasn't so good,
        // --- then our men would pick less perfect matches
        // -------------------------------------------------------
        fake_ran.SetUL( 2735976240 ); // 0.637

        IIndividualHumanSTI* p_female_match_a = p_assort->SelectPartner( p_m_h, female_list );
        IIndividualHumanSTI* p_female_match_b = p_assort->SelectPartner( p_m_v, female_list );
        IIndividualHumanSTI* p_female_match_c = p_assort->SelectPartner( p_m_k, female_list );
        IIndividualHumanSTI* p_female_match_d = p_assort->SelectPartner( p_m_a, female_list );
        IIndividualHumanSTI* p_female_match_e = p_assort->SelectPartner( p_m_r, female_list );

        CHECK( p_f_v == p_female_match_a );
        CHECK( p_f_v == p_female_match_b );
        CHECK( p_f_k == p_female_match_c );
        CHECK( p_f_a == p_female_match_d );
        CHECK( p_f_r == p_female_match_e );
    }

    void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        try
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

            RandomFake fake_ran ;
            unique_ptr<IAssortivity> p_assort( AssortivityFactory::CreateAssortivity( RelationshipType::TRANSITORY, &fake_ran ) );

            p_assort->Configure( p_config.get() );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug( msg );
                CHECK_LN( false, lineNumber );
            }
        }
    }

    TEST_FIXTURE(AssortivityFixture, TestBadAxesNameSti)
    {
        m_pSimulationConfig->sim_type = SimType::STI_SIM ;

        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadAxesNameSti.json",
            "The TRANSITORY:Group (STI_INFECTION_STATUS) requires that the Axes names(='FALSE' 'XXX' ) are 'TRUE' and 'FALSE'.  Order is up to the user." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadAxesNameHiv)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadAxesNameHiv.json",
            "The TRANSITORY:Group (HIV_INFECTION_STATUS) requires that the Axes names(='XXX' 'TRUE' ) are 'TRUE' and 'FALSE'.  Order is up to the user." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadAxesNamePropertyA)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadAxesNamePropertyA.json",
            "The TRANSITORY:Group (INDIVIDUAL_PROPERTY) requires that the Axes names(='HUMAN' 'VULCAN' 'KLINGON' '' 'ROMULAN' ) match the property values(=ANDORIAN, HUMAN, KLINGON, ROMULAN, VULCAN) defined in the demographics for Property=Race." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadAxesNamePropertyB)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadAxesNamePropertyB.json",
            "The TRANSITORY:Group (INDIVIDUAL_PROPERTY) requires that the Axes names(='HUMAN' 'VULCAN' 'KLINGON' 'XXX' 'ROMULAN' ) match the property values(=ANDORIAN, HUMAN, KLINGON, ROMULAN, VULCAN) defined in the demographics for Property=Race." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadAxesNamePropertyC)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadAxesNamePropertyC.json",
            "The TRANSITORY:Group (INDIVIDUAL_PROPERTY) requires that the Axes names(='HUMAN' 'VULCAN' 'KLINGON' 'ROMULAN' ) match the property values(=ANDORIAN, HUMAN, KLINGON, ROMULAN, VULCAN) defined in the demographics for Property=Race." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadPropertyName)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadPropertyName.json",
            "TRANSITORY:Property_Name must be defined and cannot be empty string." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadMatrixBadRows)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadMatrixBadRows.json",
            "The TRANSITORY:Weighting Matrix must be a square matrix whose dimensions are equal to the number of Axes." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadMatrixBadColumn)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadMatrixBadColumn.json",
            "The TRANSITORY:Weighting Matrix must be a square matrix whose dimensions are equal to the number of Axes." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadMatrixValue)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadMatrixValue.json",
            "Configuration variable Weighting_Matrix_RowMale_ColumnFemale with value 9999 out of range: greater than 1.\nWas reading values for TRANSITORY." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadStartYear)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadStartYear.json",
            "Configuration variable Start_Year with value 9999 out of range: greater than 2200.\nWas reading values for TRANSITORY." );
    }

    TEST_FIXTURE(AssortivityFixture, TestMissingStartYear)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestMissingStartYear.json",
            "Parameter 'Start_Year of AssortivityHIV' not found in input file 'testdata/AssortivityTest/TestMissingStartYear.json'.\n\nWas reading values for TRANSITORY." );
    }

    TEST_FIXTURE(AssortivityFixture, TestMissingGroup)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestMissingGroup.json",
            "While trying to parse json data for param/key >>> Group <<< in otherwise valid json segment" );
    }

    TEST_FIXTURE(AssortivityFixture, TestMatrixRowAllZeros)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestMatrixRowAllZeros.json",
            "The TRANSITORY:Weighting Matrix cannot have a row or column where all the values are zero.  Row 4 is all zeros." );
    }

    TEST_FIXTURE(AssortivityFixture, TestMatrixColumnAllZeros)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestMatrixColumnAllZeros.json",
            "The TRANSITORY:Weighting Matrix cannot have a row or column where all the values are zero.  Column 3 is all zeros." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadAxesNamesReceivedResultsA)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadAxesNamesReceivedResultsA.json",
            "The TRANSITORY:Group (HIV_RECEIVED_RESULTS_STATUS) requires that the Axes names(='UNKNOWN' 'POSITIVE' ) are 'UNKNOWN' 'POSITIVE' 'NEGATIVE' .  Order is up to the user." );
    }

    TEST_FIXTURE(AssortivityFixture, TestBadAxesNamesReceivedResultsB)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/TestBadAxesNamesReceivedResultsB.json",
            "The TRANSITORY:Group (HIV_RECEIVED_RESULTS_STATUS) requires that the Axes names(='UNKNOWN' 'POSITIVE' 'XXX' ) are 'UNKNOWN' 'POSITIVE' 'NEGATIVE' .  Order is up to the user." );
    }

    TEST_FIXTURE(AssortivityFixture, TestInvalidGroupForSimType)
    {
        m_pSimulationConfig->sim_type = SimType::STI_SIM ;

        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/HivInfectionStatus.json",
            "Variable or parameter 'TRANSITORY:Group' with value HIV_INFECTION_STATUS is incompatible with variable or parameter 'Simulation_Type' with value STI_SIM. HIV_INFECTION_STATUS is only valid with HIV_SIM." );
    }

    TEST_FIXTURE(AssortivityFixture, TestInvalidGroupForSimType2)
    {
        m_pSimulationConfig->sim_type = SimType::HIV_SIM ;

        TestHelper_ConfigureException( __LINE__, "testdata/AssortivityTest/StiInfectionStatus.json",
            "Variable or parameter 'TRANSITORY:Group' with value STI_INFECTION_STATUS is incompatible with variable or parameter 'Simulation_Type' with value HIV_SIM. STI_INFECTION_STATUS is only valid with STI_SIM." );
    }
}
