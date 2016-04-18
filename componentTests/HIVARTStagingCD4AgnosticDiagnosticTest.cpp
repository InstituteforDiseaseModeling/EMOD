/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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

            m_InterventionsContext.setCascadeState( "not_set" );
            m_InterventionsContext.SetContextTo( &m_Human );
            m_Diag.SetContextTo( &m_Human );

            m_pSimulationConfig->sim_type = SimType::HIV_SIM ;
            m_pSimulationConfig->listed_events.insert("Births"          );
            m_pSimulationConfig->listed_events.insert("NonDiseaseDeaths");

            IdmDateTime idm_time ;
            idm_time.time = 2009.0 * 365.0;
            m_NEC.SetTime( idm_time );

            m_Human.SetHasHIV( true );

            Node::TestOnly_AddPropertyKeyValue( "HasActiveTB", "YES" );
            Node::TestOnly_AddPropertyKeyValue( "HasActiveTB", "NO" );
        }

        ~DiagnosticFixture()
        {
            delete m_pSimulationConfig;
            Environment::Finalize();
            Node::TestOnly_ClearProperties();
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
        m_Human.GetProperties()->operator[]( "HasActiveTB" ) = "NO" ;
        m_Human.SetIsPregnant( false );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;


        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::Births, m_NEC.GetTriggeredEvent() ) ;
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
        m_Human.GetProperties()->operator[]( "HasActiveTB" ) = "YES" ;
        m_Human.SetIsPregnant( false );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;


        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::Births, m_NEC.GetTriggeredEvent() ) ;
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
        m_Human.GetProperties()->operator[]( "HasActiveTB" ) = "NO" ;
        m_Human.SetIsPregnant( false );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;

        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::Births, m_NEC.GetTriggeredEvent() ) ;
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
        m_Human.GetProperties()->operator[]( "HasActiveTB" ) = "NO" ;
        m_Human.SetIsPregnant( false );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;

        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::Births, m_NEC.GetTriggeredEvent() ) ;
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
        m_Human.GetProperties()->operator[]( "HasActiveTB" ) = "NO" ;
        m_Human.SetIsPregnant( false );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;

        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::Births, m_NEC.GetTriggeredEvent() ) ;
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
        m_Human.GetProperties()->operator[]( "HasActiveTB" ) = "NO" ;
        m_Human.SetIsPregnant( false );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;

        CHECK_EQUAL( false, m_Human.IsPregnant() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::NonDiseaseDeaths, m_NEC.GetTriggeredEvent() ) ;
        CHECK_EQUAL( 2.1f,  m_InterventionsContext.LastRecordedWHOStage() );
        CHECK_EQUAL( true,  m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( m_Diag.Expired() );
    }
}
