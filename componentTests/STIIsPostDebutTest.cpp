/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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
            m_InterventionsContext.setCascadeState( "not_set" );
            m_InterventionsContext.SetContextTo( &m_Human );

            Environment::setSimulationConfig( m_pSimulationConfig );
            m_pSimulationConfig->listed_events.insert( "Births"   );
            m_pSimulationConfig->listed_events.insert( "NonDiseaseDeaths" );

			m_pDiag = new STIIsPostDebut();
			m_pDiag->SetContextTo( &m_Human );
        }

        ~DiagnosticFixture()
        {
            m_pSimulationConfig->listed_events.clear();
            delete m_pSimulationConfig;
            Environment::setSimulationConfig( nullptr );
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

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;
        CHECK( !m_pDiag->Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_pDiag->Distribute( &m_InterventionsContext, &cco_fake );
        CHECK( distributed );
        CHECK( m_pDiag->Expired() );

        m_pDiag->Update( 1.0 );

        CHECK_EQUAL( IndividualEventTriggerType::Births, m_NEC.GetTriggeredEvent() ) ;
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

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;
        CHECK( !m_pDiag->Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_pDiag->Distribute( &m_InterventionsContext, &cco_fake );
        CHECK( distributed );

        CHECK_EQUAL( IndividualEventTriggerType::NonDiseaseDeaths, m_NEC.GetTriggeredEvent() ) ;
        CHECK( m_pDiag->Expired() );
    }

}
