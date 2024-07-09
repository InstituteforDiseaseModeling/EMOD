

#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "componentTests.h"
#include "MigrateIndividuals.h"
#include "Individual.h"

#include "FileSystem.h"
#include "Configuration.h"
#include "SimulationConfig.h"

using namespace Kernel;

SUITE(MigrateIndividualsTest)
{
    struct MigrateFixture
    {
        MigrateIndividuals  m_MigrateIndividuals;
        SimulationConfig*   m_pSimulationConfig ;

        MigrateFixture()
            : m_MigrateIndividuals()
            , m_pSimulationConfig( new SimulationConfig() )
        {
            JsonConfigurable::_useDefaults = true;
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            m_pSimulationConfig->migration_structure = MigrationStructure::NO_MIGRATION;
        }

        ~MigrateFixture()
        {
            IndividualHumanConfig::migration_pattern = MigrationPattern::RANDOM_WALK_DIFFUSION;
            delete m_pSimulationConfig;
            Environment::Finalize();
        }
    };

    TEST_FIXTURE(MigrateFixture, TestPositive)
    {
        // -------------------------------------------------------------
        // --- Test that the intervention Configures() ok when
        // --- the migration_structure is valid.
        // -------------------------------------------------------------
        m_pSimulationConfig->migration_structure = MigrationStructure::FIXED_RATE_MIGRATION;
        IndividualHumanConfig::migration_pattern = MigrationPattern::SINGLE_ROUND_TRIPS;
        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/MigrateIndividualsTest.json" ) );
        m_MigrateIndividuals.Configure( p_config.get() );
    }

    TEST_FIXTURE(MigrateFixture, TestNegative)
    {
        // -------------------------------------------------------------
        // --- Test that the intervention Configures() ok when
        // --- the migration_structure is valid.
        // -------------------------------------------------------------
        try
        {
            m_pSimulationConfig->migration_structure = MigrationStructure::NO_MIGRATION;
            std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/MigrateIndividualsTest.json" ) );
            m_MigrateIndividuals.Configure( p_config.get() );
            CHECK( false ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            bool passed = msg.find( "MigrateIndividuals cannot be used when 'Migration_Model' = 'NO_MIGRATION'." ) != string::npos ;
            if( !passed )
            {
                PrintDebug( msg );
            }
            CHECK( passed );
        }

    }
}
