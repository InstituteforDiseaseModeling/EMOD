/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "HIVARTStagingByCD4Diagnostic.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "ICampaignCostObserverFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "ISusceptibilityHIVFake.h"

#include "FileSystem.h"
#include "Configuration.h"
#include "Simulation.h"
#include "Node.h"
#include "SimulationConfig.h"

using namespace Kernel;

SUITE(HivArtStagingByCD4DiagnosticTest)
{
    struct DiagnosticFixture
    {
        INodeContextFake                        m_NC ;
        INodeEventContextFake                   m_NEC ;
        IndividualHumanInterventionsContextFake m_InterventionsContext ;
        IndividualHumanContextFake              m_Human ;
        HIVARTStagingByCD4Diagnostic            m_Diag ;
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
            Environment::setSimulationConfig( m_pSimulationConfig );

            IdmDateTime idm_time ;
            idm_time.time = 2009.0 * 365.0;
            m_NEC.SetTime( idm_time );

            Node::TestOnly_AddPropertyKeyValue( "HasActiveTB", "YES" );
            Node::TestOnly_AddPropertyKeyValue( "HasActiveTB", "NO" );

            m_Human.GetProperties()->operator[]( "HasActiveTB" ) = "YES" ;
            m_Human.SetHasHIV( true );

            m_pSimulationConfig->sim_type = SimType::HIV_SIM ;
            m_pSimulationConfig->listed_events.insert("Births"          );
            m_pSimulationConfig->listed_events.insert("NonDiseaseDeaths");
        }

        ~DiagnosticFixture()
        {
            delete m_pSimulationConfig;
            Environment::Finalize();
            Node::TestOnly_ClearProperties();
        }
    };

    TEST_FIXTURE(DiagnosticFixture, TestPositive)
    {
        // -------------------------------------------------------------
        // --- Test that we get a positive test result which gives us
        // --- a positive test distribution. To get things positive, 
        // --- we need the CD4Count to be below a threshold for the 
        // --- currnet year given the individuals current health status.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HivArtStagingByCD4DiagnosticTest.json" ) );
        m_Diag.Configure( p_config.get() );

        m_InterventionsContext.OnTestCD4( 333.0 );
        m_Human.GetProperties()->operator[]( "HasActiveTB" ) = "YES" ;
        m_Human.SetIsPregnant( true );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;

        CHECK_EQUAL( true,  m_Human.IsPregnant() );
        CHECK_EQUAL( true,  m_InterventionsContext.EverReceivedCD4() );
        CHECK_EQUAL( 333.0, m_InterventionsContext.LastRecordedCD4() );
        CHECK_EQUAL( 333.0, m_InterventionsContext.LowestRecordedCD4() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::Births, m_NEC.GetTriggeredEvent() ) ;
        CHECK_EQUAL( true,  m_InterventionsContext.EverReceivedCD4() );
        CHECK_EQUAL( 333.0, m_InterventionsContext.LastRecordedCD4() );
        CHECK_EQUAL( 333.0, m_InterventionsContext.LowestRecordedCD4() );
        CHECK_EQUAL( true,  m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( true,  m_InterventionsContext.EverStagedForART() );

        CHECK( m_Diag.Expired() );
    }

    TEST_FIXTURE(DiagnosticFixture, TestNegative)
    {
        // -------------------------------------------------------------
        // --- Test that we get a negative test result which gives us
        // --- a negative test distribution. To get things negative, 
        // --- we need the CD4Count to be above the thresholds for the 
        // --- currnet year given the individuals current health status.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HivArtStagingByCD4DiagnosticTest.json" ) );
        m_Diag.Configure( p_config.get() );

        m_InterventionsContext.OnTestCD4( 999.0 );
        m_Human.GetProperties()->operator[]( "HasActiveTB" ) = "YES" ;
        m_Human.SetIsPregnant( true );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;

        CHECK_EQUAL( true,  m_Human.IsPregnant() );
        CHECK_EQUAL( true,  m_InterventionsContext.EverReceivedCD4() );
        CHECK_EQUAL( 999.0, m_InterventionsContext.LastRecordedCD4() );
        CHECK_EQUAL( 999.0, m_InterventionsContext.LowestRecordedCD4() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::NonDiseaseDeaths, m_NEC.GetTriggeredEvent() ) ;
        CHECK_EQUAL( true,  m_InterventionsContext.EverReceivedCD4() );
        CHECK_EQUAL( 999.0, m_InterventionsContext.LastRecordedCD4() );
        CHECK_EQUAL( 999.0, m_InterventionsContext.LowestRecordedCD4() );
        CHECK_EQUAL( true,  m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( m_Diag.Expired() );
    }

    TEST_FIXTURE(DiagnosticFixture, TestEarlyYear)
    {
        // -------------------------------------------------------------
        // --- This test is similar to TestPositive except that the year is
        // --- made earlier to test using the default value.  The default value
        // --- causes the test to fail so that a negative reponse is made.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HivArtStagingByCD4DiagnosticTest.json" ) );
        m_Diag.Configure( p_config.get() );

        // The input file has the eariest year at 2000.
        IdmDateTime idm_time ;
        idm_time.time = 1990.0 * 365.0;
        m_NEC.SetTime( idm_time );

        m_InterventionsContext.OnTestCD4( 333.0 );
        m_Human.GetProperties()->operator[]( "HasActiveTB" ) = "YES" ;
        m_Human.SetIsPregnant( true );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;

        CHECK_EQUAL( true,  m_Human.IsPregnant() );
        CHECK_EQUAL( true,  m_InterventionsContext.EverReceivedCD4() );
        CHECK_EQUAL( 333.0, m_InterventionsContext.LastRecordedCD4() );
        CHECK_EQUAL( 333.0, m_InterventionsContext.LowestRecordedCD4() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::NonDiseaseDeaths, m_NEC.GetTriggeredEvent() ) ;
        CHECK_EQUAL( true,  m_InterventionsContext.EverReceivedCD4() );
        CHECK_EQUAL( 333.0, m_InterventionsContext.LastRecordedCD4() );
        CHECK_EQUAL( 333.0, m_InterventionsContext.LowestRecordedCD4() );
        CHECK_EQUAL( true,  m_InterventionsContext.EverStaged() );
        CHECK_EQUAL( false, m_InterventionsContext.EverStagedForART() );

        CHECK( m_Diag.Expired() );
    }

}
