
#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>
#include "componentTests.h"

#include "VectorControlNodeTargeted.h"
#include "Configuration.h"
#include "SimulationConfig.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"

using namespace Kernel;

SUITE(SugarTrapTest)
{
    struct TrapFixture
    {
        SimulationConfig* m_pSimulationConfig ;

        TrapFixture()
            : m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            try
            {
                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/SugarTrapTest/config.json" ) );
                m_pSimulationConfig->vector_params->vector_species.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params" );
                m_pSimulationConfig->vector_params->vector_species.CheckConfiguration();
            }
            catch( DetailedException& re )
            {
                PrintDebug( re.GetMsg() );
                throw re;
            }
        }

        ~TrapFixture()
        {
            delete m_pSimulationConfig;
            Environment::setSimulationConfig( nullptr );
            Environment::Finalize();
        }

        void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
        {
            try
            {
                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

                SugarTrap trap;
                bool ret = trap.Configure( p_config.get() );

                CHECK_LN( false, lineNumber ); // should not get here
            }
            catch( DetailedException& re )
            {
                std::string msg = re.GetMsg();
                if( msg.find( rExpMsg ) == string::npos )
                {
                    PrintDebug( rExpMsg );
                    PrintDebug( msg );
                    CHECK_LN( false, lineNumber );
                }
            }
        }
    };

    TEST_FIXTURE( TrapFixture, TestFrequencyNone )
    {
        std::stringstream ss;
        ss << "Using 'SugarTrap' intervention but 'Vector_Sugar_Feeding_Frequency' set to 'VECTOR_SUGAR_FEEDING_NONE'\n"
           << "for all the species.  'Vector_Sugar_Feeding_Frequency' must be set to something besides\n"
           << "'VECTOR_SUGAR_FEEDING_NONE' for at least one specie when using 'SugarTrap'.\n"
           << "Options are:\n"
           << "VECTOR_SUGAR_FEEDING_NONE\n"
           << "VECTOR_SUGAR_FEEDING_ON_EMERGENCE_ONLY\n"
           << "VECTOR_SUGAR_FEEDING_EVERY_FEED\n"
           << "VECTOR_SUGAR_FEEDING_EVERY_DAY\n";
        TestHelper_ConfigureException( __LINE__, "testdata/SugarTrapTest/TestFrequencyNone.json", ss.str() );
    }
}
