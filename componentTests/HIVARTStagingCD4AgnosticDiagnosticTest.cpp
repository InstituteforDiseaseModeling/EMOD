/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "HIVARTStagingCD4AgnosticDiagnostic.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "ICampaignCostObserverFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"

#include "FileSystem.h"
#include "Configuration.h"
#include "Simulation.h"
#include "Node.h"
#include "SimulationConfig.h"

using namespace Kernel;

SUITE(HivArtStagingCD4AgnosticDiagnosticTest)
{
    struct DiagnosticFixture
    {
        INodeContextFake                        m_NC ;
        INodeEventContextFake                   m_NEC ;
        IndividualHumanInterventionsContextFake m_InterventionsContext ;
        IndividualHumanContextFake              m_Human ;
        HIVARTStagingCD4AgnosticDiagnostic      m_Diag ;
        SimulationConfig*                       m_pSimulationConfig ;

        DiagnosticFixture()
            : m_NC()
            , m_NEC()
            , m_InterventionsContext()
            , m_Human( &m_InterventionsContext, &m_NC, &m_NEC, nullptr )
            , m_Diag()
            , m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            m_InterventionsContext.SetContextTo( &m_Human );
            m_Diag.SetContextTo( &m_Human );

            m_pSimulationConfig->sim_type = SimType::HIV_SIM ;

            EventTriggerFactory::DeleteInstance();

            json::Object fakeConfigJson;
            Configuration * fakeConfigValid = Environment::CopyFromElement( fakeConfigJson );
            EventTriggerFactory::GetInstance()->Configure( fakeConfigValid );
            m_NEC.Initialize();

            IdmDateTime idm_time ;
            idm_time.time = 2009.0 * 365.0;
            m_NEC.SetTime( idm_time );

            std::map<std::string, float> ip_values_state ;
            ip_values_state.insert( std::make_pair( "abort_state_1",   0.0f ) );
            ip_values_state.insert( std::make_pair( "abort_state_2",   0.0f ) );
            ip_values_state.insert( std::make_pair( "abort_state_3",   0.0f ) );
            ip_values_state.insert( std::make_pair( "non_abort_state", 0.0f ) );
            ip_values_state.insert( std::make_pair( "no_state",        1.0f ) );

            std::map<std::string,float> ip_values ;
            ip_values.insert( std::make_pair( "YES", 0.5f ) );
            ip_values.insert( std::make_pair( "NO",  0.5f ) );

            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();
            IPFactory::GetInstance()->AddIP( 1, "HasActiveTB", ip_values );
            IPFactory::GetInstance()->AddIP( 1, "InterventionStatus", ip_values_state );

            m_Human.SetHasHIV( true );
            m_Human.GetProperties()->Add( IPKeyValue( "InterventionStatus:no_state" ) );
        }

        ~DiagnosticFixture()
        {
            delete m_pSimulationConfig;
            Environment::setSimulationConfig( nullptr );
            IPFactory::DeleteFactory();
            Environment::Finalize();
        }
    };

    TEST_FIXTURE(DiagnosticFixture, TestPositiveAdultWhoStage)
    {
        // -------------------------------------------------------------
        // --- Test that a person that is an adult and with the valid WHO Stage
        // --- results in a positive result and distribution.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HivArtStagingCD4AgnosticDiagnosticTest.json" ) );
        m_Diag.Configure( p_config.get() );

        m_Human.SetAge( 22*365 ); // Adult
        m_Human.SetWhoStage( 2.1f ); // WHO stage > 2005 stage = 2
        m_Human.GetProperties()->Add( IPKeyValue( "HasActiveTB:NO" ) );
        m_Human.SetIsPregnant( false );

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() ) ;


        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( EventTrigger::Births.ToString(), m_NEC.GetTriggeredEvent().ToString() ) ;
        CHECK_EQUAL( 2.1f, m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( true, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( true, m_InterventionsContext.EverStagedForART() );

        CHECK( m_Diag.Expired() );
    }

    TEST_FIXTURE(DiagnosticFixture, TestPositiveAdultHasTB)
    {
        // -------------------------------------------------------------
        // --- Test that a person that is an adult, with invalid WHO stage,
        // --- but has TB results in a positive result and distribution.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HivArtStagingCD4AgnosticDiagnosticTest.json" ) );
        m_Diag.Configure( p_config.get() );

        m_Human.SetAge( 22*365 ); // Adult
        m_Human.SetWhoStage( 1.5f ); // WHO stage < 2005 stage = 2
        m_Human.GetProperties()->Add( IPKeyValue( "HasActiveTB:YES" ) );
        m_Human.SetIsPregnant( false );

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() ) ;


        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( EventTrigger::Births.ToString(), m_NEC.GetTriggeredEvent().ToString() ) ;
        CHECK_EQUAL( 1.5,  m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( true, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( true, m_InterventionsContext.EverStagedForART() );

        CHECK( m_Diag.Expired() );
    }

    TEST_FIXTURE(DiagnosticFixture, TestPositiveChildUnderAge)
    {
        // -------------------------------------------------------------
        // --- Test that a person that is a child and with invalid WHO stage,
        // --- but with the age is less than then Treat Under Age Threshold
        // --- has a positive result and distribution.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HivArtStagingCD4AgnosticDiagnosticTest.json" ) );
        m_Diag.Configure( p_config.get() );

        m_Human.SetAge( 0.5*365 ); // child
        m_Human.SetWhoStage( 1.25f ); // WHO stage < 2005 stage = 2
        m_Human.GetProperties()->Add( IPKeyValue( "HasActiveTB:NO" ) );
        m_Human.SetIsPregnant( false );

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() ) ;

        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( EventTrigger::Births.ToString(), m_NEC.GetTriggeredEvent().ToString() ) ;
        CHECK_EQUAL( 1.25,  m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( true, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( true, m_InterventionsContext.EverStagedForART() );

        CHECK( m_Diag.Expired() );
    }

    TEST_FIXTURE(DiagnosticFixture, TestPositiveChildWhoStage)
    {
        // -------------------------------------------------------------
        // --- Test that a person that is a child and with valid WHO stage
        // --- has a positive result and distribution.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HivArtStagingCD4AgnosticDiagnosticTest.json" ) );
        m_Diag.Configure( p_config.get() );

        m_Human.SetAge( 1.5*365 ); // child
        m_Human.SetWhoStage( 1.75f ); // WHO stage > 2005 child stage = 1.5
        m_Human.GetProperties()->Add( IPKeyValue( "HasActiveTB:NO" ) );
        m_Human.SetIsPregnant( false );

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() ) ;

        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( EventTrigger::Births.ToString(), m_NEC.GetTriggeredEvent().ToString() ) ;
        CHECK_EQUAL( 1.75,  m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( true, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( true, m_InterventionsContext.EverStagedForART() );

        CHECK( m_Diag.Expired() );
    }

    TEST_FIXTURE(DiagnosticFixture, TestNegativeAdult)
    {
        // -------------------------------------------------------------
        // --- Test that a person that is a child and with valid WHO stage
        // --- has a positive result and distribution.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HivArtStagingCD4AgnosticDiagnosticTest.json" ) );
        m_Diag.Configure( p_config.get() );

        m_Human.SetAge( 22*365 ); // Adult
        m_Human.SetWhoStage( 1.5f ); // WHO stage M 2005 stage = 2
        m_Human.GetProperties()->Add( IPKeyValue( "HasActiveTB:NO" ) );
        m_Human.SetIsPregnant( false );

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() ) ;

        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( EventTrigger::Births.ToString(), m_NEC.GetTriggeredEvent().ToString() ) ;
        CHECK_EQUAL( 1.5,  m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( true, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( true, m_InterventionsContext.EverStagedForART() );

        CHECK( m_Diag.Expired() );
    }

    TEST_FIXTURE(DiagnosticFixture, TestEarlyYear)
    {
        // -------------------------------------------------------------
        // --- This test is similar to TestPositiveAdultWhoStage but the year is earlier
        // --- than the data.  This causes the default values to be used
        // --- which causes the test to fail and a negative response made.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HivArtStagingCD4AgnosticDiagnosticTest.json" ) );
        m_Diag.Configure( p_config.get() );

        // The input file has the eariest year at 2000.
        IdmDateTime idm_time ;
        idm_time.time = 1990.0 * 365.0;
        m_NEC.SetTime( idm_time );

        m_Human.SetAge( 22*365 ); // Adult
        m_Human.SetWhoStage( 2.1f ); // WHO stage > 2005 stage = 2
        m_Human.GetProperties()->Add( IPKeyValue( "HasActiveTB:NO" ) );
        m_Human.SetIsPregnant( false );

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() ) ;

        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( EventTrigger::NonDiseaseDeaths.ToString(), m_NEC.GetTriggeredEvent().ToString() ) ;
        CHECK_EQUAL( 2.1f,  m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( true,  m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( m_Diag.Expired() );
    }
}
