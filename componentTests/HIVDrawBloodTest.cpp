/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "HIVDrawBlood.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "ICampaignCostObserverFake.h"
#include "ISusceptibilityHIVFake.h"
#include "Configuration.h"
#include "Simulation.h"
#include "SimulationConfig.h"

using namespace Kernel;

SUITE(HivDrawBloodTest)
{
    struct DiagnosticFixture
    {
        INodeContextFake                        m_NC ;
        INodeEventContextFake                   m_NEC ;
        IndividualHumanInterventionsContextFake m_InterventionsContext ;
        ISusceptibilityHIVFake                  m_ISusceptibilityHIVFake ;
        IndividualHumanContextFake              m_Human ;
        HIVDrawBlood                            m_Diag ;
        SimulationConfig* m_pSimulationConfig ;

        DiagnosticFixture()
            : m_NC()
            , m_NEC()
            , m_InterventionsContext()
            , m_ISusceptibilityHIVFake()
            , m_Human( &m_InterventionsContext, &m_NC, &m_NEC, &m_ISusceptibilityHIVFake )
            , m_Diag()
            , m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            m_InterventionsContext.setCascadeState( "not_set" );
            m_InterventionsContext.SetContextTo( &m_Human );
            m_Diag.SetContextTo( &m_Human );
            m_ISusceptibilityHIVFake.SetCD4Count( 777.0 );
            m_pSimulationConfig->sim_type = SimType::HIV_SIM ;
            m_pSimulationConfig->listed_events.insert("Births"   );
            m_pSimulationConfig->listed_events.insert("NoTrigger");
        }

        ~DiagnosticFixture()
        {
            m_pSimulationConfig->listed_events.clear();
            delete m_pSimulationConfig;
            Environment::setSimulationConfig( nullptr );
            Environment::Finalize();
        }
    };

    TEST_FIXTURE(DiagnosticFixture, TestDistribute)
    {
        // -------------------------------------------------------------
        // --- Test that the CD4 count is updated.
        // -------------------------------------------------------------
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/HIVDrawBloodTest.json" ) );
        m_Diag.Configure( p_config.get() );

        CHECK_EQUAL( IndividualEventTriggerType::NoTrigger, m_NEC.GetTriggeredEvent() ) ;

        CHECK_EQUAL( false, m_InterventionsContext.EverReceivedCD4() );
        CHECK_EQUAL( 0.0,   m_InterventionsContext.LastRecordedCD4() );

        CHECK( !m_Diag.Expired() );

        ICampaignCostObserverFake cco_fake ;
        bool distributed = m_Diag.Distribute( &m_InterventionsContext, &cco_fake );

        CHECK( distributed );

        m_Diag.Update(1.0);

        CHECK_EQUAL( IndividualEventTriggerType::Births, m_NEC.GetTriggeredEvent() ) ;
        CHECK_EQUAL( true,  m_InterventionsContext.EverReceivedCD4() );
        CHECK_EQUAL( 777.0, m_InterventionsContext.LastRecordedCD4() );

        CHECK( m_Diag.Expired() );
    }
}
