
#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "componentTests.h"
#include "INodeContextFake.h"

#include "VectorInterventionsContainer.h"

#include "RANDOM.h"
#include "SimulationConfig.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"

#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "IndividualHumanContextFake.h"

using namespace Kernel;

SUITE( VectorInterventionsContainerTest )
{
    struct VectorInterventionsContainerFixture
    {
        SimulationConfig*          m_pSimulationConfig;
        INodeEventContextFake      m_NEC;
        INodeContextFake           m_NC;
        IndividualHumanContextFake m_Human;

        VectorInterventionsContainerFixture()
            : m_pSimulationConfig( new SimulationConfig() )
            , m_NEC()
            , m_NC(1,&m_NEC)
            , m_Human( nullptr, &m_NC, &m_NEC, nullptr )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            m_pSimulationConfig->vector_params->vector_aging = false;
            m_pSimulationConfig->vector_params->human_feeding_mortality = 0.0;

            try
            {
                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorInterventionsContainerTest_config.json" ) );
                m_pSimulationConfig->vector_params->vector_species.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params" );
                m_pSimulationConfig->vector_params->vector_species.CheckConfiguration();
            }
            catch( DetailedException& re )
            {
                PrintDebug( re.GetMsg() );
                throw re;
            }
        }

        ~VectorInterventionsContainerFixture()
        {
            Environment::Finalize();
        }
    };

    TEST_FIXTURE( VectorInterventionsContainerFixture, TestProbabilities )
    {
        float EPSILON = 0.000001f;
        VectorInterventionsContainer vic;
        vic.SetContextTo( &m_Human );

        vic.InfectiousLoopUpdate( 1.0f );
        vic.Update( 1.0f );

        CHECK_CLOSE( 0.0f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 1.0f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetSuccessfulFeedAD().GetDefaultValue()   , EPSILON );

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfBlocking( 0.6f );
        vic.UpdateProbabilityOfKilling( 0.0f );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.0f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.6f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.4f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetSuccessfulFeedAD().GetDefaultValue()   , EPSILON );

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfBlocking( 0.0f );
        vic.UpdateProbabilityOfKilling( 0.3f );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.0f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 1.0f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetSuccessfulFeedAD().GetDefaultValue()   , EPSILON );

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfBlocking( 0.6f );
        vic.UpdateProbabilityOfKilling( 0.3f );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.18f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.42f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.40f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetSuccessfulFeedAD().GetDefaultValue()   , EPSILON );
    }
}
