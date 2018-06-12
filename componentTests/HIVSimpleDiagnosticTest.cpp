/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "HIVSimpleDiagnostic.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "ICampaignCostObserverFake.h"

#include "FileSystem.h"
#include "Configuration.h"
#include "Simulation.h"
#include "SimulationConfig.h"

using namespace Kernel;

SUITE(HivSimpleDiagnosticTest)
{
    struct DiagnosticFixture
    {
        INodeContextFake                        m_NC ;
        INodeEventContextFake                   m_NEC ;
        IndividualHumanInterventionsContextFake m_InterventionsContext ;
        IndividualHumanContextFake              m_Human ;
        HIVSimpleDiagnostic                     m_Diag ;
        SimulationConfig* m_pSimulationConfig ;

        DiagnosticFixture()
            : m_NEC()
            , m_InterventionsContext()
            , m_Human( &m_InterventionsContext, &m_NC, &m_NEC, nullptr )
            , m_Diag()
            , m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            m_pSimulationConfig->sim_type = SimType::HIV_SIM ;

            EventTriggerFactory::DeleteInstance();

            json::Object fakeConfigJson;
            Configuration * fakeConfigValid = Environment::CopyFromElement( fakeConfigJson );
            EventTriggerFactory::GetInstance()->Configure( fakeConfigValid );
            m_NEC.Initialize();

            std::map<std::string, float> ip_values_state ;
            ip_values_state.insert( std::make_pair( "abort_state_1", 0.0f ) );
            ip_values_state.insert( std::make_pair( "abort_state_2", 0.0f ) );
            ip_values_state.insert( std::make_pair( "abort_state_3", 0.0f ) );
            ip_values_state.insert( std::make_pair( "non_abort_state", 0.0f ) );
            ip_values_state.insert( std::make_pair( "no_state", 1.0f ) );

            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();
            IPFactory::GetInstance()->AddIP( 1, "InterventionStatus", ip_values_state );

            m_InterventionsContext.SetContextTo( &m_Human );

            m_Diag.SetContextTo( &m_Human );

            m_Human.SetHasHIV( true );
            m_Human.GetProperties()->Add( IPKeyValue( "InterventionStatus:no_state" ) );
        }

        ~DiagnosticFixture()
        {
            delete m_pSimulationConfig;
            Environment::setSimulationConfig( nullptr );
            Environment::Finalize();
        }
    };

    TEST_FIXTURE(DiagnosticFixture, TestDisqualifyingPropertiesAndInterventionStatus)
    {
        // -------------------------------------------------------------
        // --- Test that current intervention state is NOT in invalid states so
        // --- diagnostic does NOT expire
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HIVSimpleDiagnosticTest/TestDisqualifyingPropertiesAndInterventionStatus.json" ) );
        m_Diag.Configure( p_config.get() ); 

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() );
        CHECK_EQUAL( std::string("no_state"), m_Human.GetProperties()->Get( IPKey("InterventionStatus" ) ).GetValueAsString() );
        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );
        CHECK( !m_Diag.Expired() );

        m_Diag.Update( 1.0 ); // call during the same timestep as Distribute

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() );
        CHECK_EQUAL( std::string( "non_abort_state" ), m_Human.GetProperties()->Get( IPKey( "InterventionStatus" ) ).GetValueAsString() );
        CHECK( !m_Diag.Expired() );

        // ------------------------------------------------------------------
        // --- Test that the current intervention state IS in the invalid states so
        // --- diagnostic DOES expire and the diagnosis is not triggered.
        // ------------------------------------------------------------------
        m_Human.GetProperties()->Set( IPKeyValue( "InterventionStatus:abort_state_2" ) );

        m_Diag.Update( 1.0 );

        CHECK( !m_NEC.GetTriggeredEvent().IsUninitialized() );
        CHECK_EQUAL( EventTrigger::InterventionDisqualified.ToString(), m_NEC.GetTriggeredEvent().ToString() ) ;
        CHECK_EQUAL( std::string( "abort_state_2" ), m_Human.GetProperties()->Get( IPKey( "InterventionStatus" ) ).GetValueAsString() );
        CHECK( m_Diag.Expired() );
    }

    TEST_FIXTURE(DiagnosticFixture, TestDontDistributeDueToCurrentInterventionStatus)
    {
        // ------------------------------------------------
        // --- Test that if the current state is an abort state of the
        // --- new diagnostic, then it will not be distributed.
        // ------------------------------------------------
        m_Human.GetProperties()->Set( IPKeyValue( "InterventionStatus:abort_state_2" ) );

        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HIVSimpleDiagnosticTest/TestDisqualifyingPropertiesAndInterventionStatus.json" ) );
        m_Diag.Configure( p_config.get() );

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() );
        CHECK_EQUAL( std::string( "abort_state_2" ), m_Human.GetProperties()->Get( IPKey( "InterventionStatus" ) ).GetValueAsString() );
        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( !distributed );
        CHECK( m_Diag.Expired() );
        CHECK( !m_NEC.GetTriggeredEvent().IsUninitialized() );
        CHECK_EQUAL( EventTrigger::InterventionDisqualified.ToString(), m_NEC.GetTriggeredEvent().ToString() ) ;
        CHECK_EQUAL( std::string( "abort_state_2" ), m_Human.GetProperties()->Get( IPKey( "InterventionStatus" ) ).GetValueAsString() );
    }
}
