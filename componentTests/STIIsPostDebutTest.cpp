/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/
#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "STIIsPostDebut.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "ICampaignCostObserverFake.h"
#include "Configuration.h"
#include "Simulation.h"
#include "SimulationConfig.h"

using namespace Kernel;

SUITE(StiIsPostDebutTest)
{
    struct DiagnosticFixture
    {
        INodeContextFake                        m_NC ;
        INodeEventContextFake                   m_NEC ;
        IndividualHumanInterventionsContextFake m_InterventionsContext ;
        IndividualHumanContextFake              m_Human ;
        SimulationConfig*                       m_pSimulationConfig ;
        STIIsPostDebut                          * m_pDiag ;

        DiagnosticFixture()
            : m_NEC()
            , m_InterventionsContext()
            , m_Human( &m_InterventionsContext, &m_NC, &m_NEC, nullptr )
            , m_pSimulationConfig( new SimulationConfig() )
            //, m_Diag()
        {
            JsonConfigurable::ClearMissingParameters();
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            m_InterventionsContext.SetContextTo( &m_Human );

            std::map<std::string, float> ip_values_state ;
            ip_values_state.insert( std::make_pair( "abort_state_1", 0.0f ) );
            ip_values_state.insert( std::make_pair( "abort_state_2", 0.0f ) );
            ip_values_state.insert( std::make_pair( "abort_state_3", 0.0f ) );
            ip_values_state.insert( std::make_pair( "non_abort_state", 0.0f ) );
            ip_values_state.insert( std::make_pair( "no_state", 1.0f ) );

            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();
            IPFactory::GetInstance()->AddIP( 1, "InterventionStatus", ip_values_state );

            m_Human.GetProperties()->Add( IPKeyValue( "InterventionStatus:no_state" ) );

            EventTriggerFactory::DeleteInstance();

            json::Object fakeConfigJson;
            Configuration * fakeConfigValid = Environment::CopyFromElement( fakeConfigJson );
            EventTriggerFactory::GetInstance()->Configure( fakeConfigValid );
            m_NEC.Initialize();

            m_pDiag = new STIIsPostDebut();
            m_pDiag->SetContextTo( &m_Human );
        }

        ~DiagnosticFixture()
        {
            delete m_pSimulationConfig;
            Environment::Finalize();
            delete m_pDiag;
        }
    };

    TEST_FIXTURE(DiagnosticFixture, TestPostDebutYes)
    {
        // -------------------------------------------------------------
        // --- Test that diagnostic is distributed, tests positive, +ve event is b/cast, and expires
        // --- as the individual is post debut.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/StiIsPostDebutTest.json" ) );

        m_pDiag->Configure( p_config.get() );

        m_Human.SetAge( 25.0*365.0 );

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() ) ;
        CHECK( !m_pDiag->Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_pDiag->Distribute( &m_InterventionsContext, &cco_fake );
        CHECK( distributed );
        CHECK( m_pDiag->Expired() );

        m_pDiag->Update( 1.0 );

        CHECK_EQUAL( EventTrigger::Births.ToString(), m_NEC.GetTriggeredEvent().ToString() ) ;
    }

    TEST_FIXTURE(DiagnosticFixture, TestPostDebutNo)
    {
        // -------------------------------------------------------------
        // --- Test that diagnostic is distributed and Negative_Event is broadcast because
        // --- the individual is NOT of post debut age.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/StiIsPostDebutTest.json" ) );

        m_pDiag->Configure( p_config.get() );

        m_Human.SetAge( 1.0*365.0 );

        CHECK( m_NEC.GetTriggeredEvent().IsUninitialized() ) ;
        CHECK( !m_pDiag->Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_pDiag->Distribute( &m_InterventionsContext, &cco_fake );
        CHECK( distributed );

        CHECK_EQUAL( EventTrigger::NonDiseaseDeaths.ToString(), m_NEC.GetTriggeredEvent().ToString() ) ;
        CHECK( m_pDiag->Expired() );
    }

}
