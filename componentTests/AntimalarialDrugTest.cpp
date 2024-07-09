
#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>
#include "componentTests.h"

#include "AntiMalarialDrug.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "ICampaignCostObserverFake.h"
#include "Configuration.h"
#include "Simulation.h"
#include "MalariaDrugTypeParameters.h"
#include "StrainIdentity.h"
#include "FileSystem.h"

using namespace Kernel;

SUITE( AntimalarialDrugTest )
{
    struct AntimalarialDrugFixture
    {
        INodeContextFake                        m_NC;
        INodeEventContextFake                   m_NEC;
        IndividualHumanInterventionsContextFake m_InterventionsContext;
        IndividualHumanContextFake              m_Human;
        AntimalarialDrug                        m_Drug;

        AntimalarialDrugFixture()
            : m_NC()
            , m_NEC()
            , m_InterventionsContext()
            , m_Human( &m_InterventionsContext, &m_NC, &m_NEC, nullptr )
            , m_Drug()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );

            EnvPtr->Config = Configuration_Load( "testdata/AntimalarialDrugTest/Drugs.json" );
            MalariaDrugTypeCollection::GetInstanceNonConst()->ConfigureFromJsonAndKey( EnvPtr->Config, "Malaria_Drug_Params" );
            MalariaDrugTypeCollection::GetInstanceNonConst()->CheckConfiguration();

            m_InterventionsContext.SetContextTo( &m_Human );

            IPFactory::DeleteFactory();
            EventTriggerFactory::DeleteInstance();

            json::Object fakeConfigJson;
            Configuration * fakeConfigValid = Environment::CopyFromElement( fakeConfigJson );
            EventTriggerFactory::GetInstance()->Configure( fakeConfigValid );

            m_NEC.Initialize();
        }

        ~AntimalarialDrugFixture()
        {
            MalariaDrugTypeCollection::DeleteInstance();
            Environment::setSimulationConfig( nullptr );
            Environment::Finalize();
        }
    };

    TEST_FIXTURE( AntimalarialDrugFixture, TestDrugsOverTime )
    {
#if 0
        StrainIdentity strain;

        float inf_update = GET_CONFIG_DOUBLE( EnvPtr->Config, "Infection_Updates_Per_Timestep" );
        float dt = 1.0 / inf_update;
        float time = 0.0;

        // ------------------------------------------------------
        // ------------------------------------------------------
        std::ofstream output;
        FileSystem::OpenFileForWriting( output, "DrugsOverTime.csv", false, false );
        output << "DrugName,Time,Concentration,Efficacy,IRBC_KillRate\n";
        auto name_set = MalariaDrugTypeCollection::GetInstance()->GetDrugNames();
        for( auto drug_name : name_set )
        {
            std::string intervetion_config_name = "testdata/AntimalarialDrugTest/Test" + drug_name + ".json";
            std::unique_ptr<Configuration> p_config( Configuration_Load( intervetion_config_name ) );
            AntimalarialDrug drug;
            drug.Configure( p_config.get() );

            bool distributed = drug.Distribute( &m_InterventionsContext, nullptr );
            CHECK( distributed );
            time = 0.0;
            while( !drug.Expired() )
            {
                drug.Update( dt );
                output << drug.GetDrugName() << "," << time << "," << drug.GetDrugCurrentConcentration() << "," << drug.GetDrugCurrentEfficacy() << "," << drug.get_drug_IRBC_killrate( strain )/9.2 << "\n";
                time += dt;
            }
        }
        output.close();
#endif
    }
}
