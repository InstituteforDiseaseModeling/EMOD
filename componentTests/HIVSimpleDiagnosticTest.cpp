/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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
            m_InterventionsContext.setCascadeState( "not_set" );
            m_InterventionsContext.SetContextTo( &m_Human );
            m_Diag.SetContextTo( &m_Human );
            m_pSimulationConfig->sim_type = SimType::HIV_SIM ;
            Environment::setSimulationConfig( m_pSimulationConfig );
            m_pSimulationConfig->listed_events.insert("Births"          );
            m_pSimulationConfig->listed_events.insert("NonDiseaseDeaths");

            m_Human.SetHasHIV( true );
        }

        ~DiagnosticFixture()
        {
            m_pSimulationConfig->listed_events.clear();
            delete m_pSimulationConfig;
            Environment::setSimulationConfig( nullptr );
        }
    };

    TEST_FIXTURE(DiagnosticFixture, TestAbortStatesAndCascadeState)
    {
        // -------------------------------------------------------------
        // --- Test that current cascade state is NOT in abort states so
        // --- diagnostic does NOT expire
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HIVSimpleDiagnosticTest/TestAbortStatesAndCascadeState.json" ) );
        m_Diag.Configure( p_config.get() ); 

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ; 
        CHECK_EQUAL( std::string("not_set"), m_InterventionsContext.getCascadeState() );
        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );
        CHECK( !m_Diag.Expired() );

        m_Diag.Update( 1.0 ); // call during the same timestep as Distribute

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;
        CHECK_EQUAL( std::string("non_abort_state"), m_InterventionsContext.getCascadeState() );
        CHECK( !m_Diag.Expired() );

        // ------------------------------------------------------------------
        // --- Test that the current cascade state IS in the abort states so
        // --- diagnostic DOES expire and the diagnosis is not triggered.
        // ------------------------------------------------------------------
        m_InterventionsContext.setCascadeState( "abort_state_2" );

        m_Diag.Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::CascadeStateAborted, m_NEC.GetTriggeredEvent() ) ;
        CHECK_EQUAL( std::string("abort_state_2"), m_InterventionsContext.getCascadeState() );
        CHECK( m_Diag.Expired() );
    }

    TEST_FIXTURE(DiagnosticFixture, TestDontDistributeDueToCurrentCascadeState)
    {
        // ------------------------------------------------
        // --- Test that if the current state is an abort state of the
        // --- new diagnostic, then it will not be distributed.
        // ------------------------------------------------
        m_InterventionsContext.setCascadeState( "abort_state_2" );

        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HIVSimpleDiagnosticTest/TestAbortStatesAndCascadeState.json" ) );
        m_Diag.Configure( p_config.get() );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;
        CHECK_EQUAL( std::string("abort_state_2"), m_InterventionsContext.getCascadeState() );
        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( !distributed );
        CHECK( m_Diag.Expired() );
        CHECK_EQUAL( IndividualEventTriggerType::CascadeStateAborted, m_NEC.GetTriggeredEvent() ) ;
        CHECK_EQUAL( std::string("abort_state_2"), m_InterventionsContext.getCascadeState() );
    }
}
