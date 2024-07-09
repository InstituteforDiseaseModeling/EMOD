

#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "ARTMortalityTable.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "ICampaignCostObserverFake.h"
#include "HIVInterventionsContainerFake.h"
#include "ISusceptibilityHIVFake.h"
#include "HIVInterventionsContainer.h"

#include "FileSystem.h"
#include "Configuration.h"
#include "Simulation.h"
#include "SimulationConfig.h"
#include "componentTests.h"

using namespace Kernel;

SUITE(ARTMortalityTableTest)
{
    struct InterventionFixture
    {
        INodeContextFake                        m_NC;
        INodeEventContextFake                   m_NEC;
        IndividualHumanInterventionsContextFake m_InterventionsContext;
        IndividualHumanContextFake              m_Human;
        ARTMortalityTable                       m_ARTTable;
        SimulationConfig*                       m_pSimulationConfig;
        HIVInterventionsContainer               m_RealInterventionsContainer;
        HIVInterventionsContainerFake           m_InterventionsContainer;
        ISusceptibilityHIVFake                  m_HIVSusceptibility;
        //FakeLogger m_fakeLogger(Logger::tLevel::WARNING);

        InterventionFixture()
            : m_NEC()
            , m_InterventionsContext()
            , m_Human(&m_InterventionsContext, &m_NC, &m_NEC, nullptr)
            , m_ARTTable()
            , m_pSimulationConfig(new SimulationConfig())
            , m_RealInterventionsContainer()
            , m_InterventionsContainer()
            , m_HIVSusceptibility()
        {
            Environment::Finalize();
            //Environment::setLogger(&m_fakeLogger);

            m_InterventionsContext.SetContextTo(&m_Human);
            m_InterventionsContext.SetInterventionsContainer(&m_RealInterventionsContainer);
            m_RealInterventionsContainer.SetContextTo(&m_Human);
            m_ARTTable.SetContextTo(&m_Human);
            m_pSimulationConfig->sim_type = SimType::HIV_SIM;
            Environment::setSimulationConfig(m_pSimulationConfig);

            std::map<std::string, float> ip_values;
            ip_values.insert(std::make_pair("Low", 0.5f));
            ip_values.insert(std::make_pair("High", 0.5f));

            IPFactory::CreateFactory();
            IPFactory::GetInstance()->AddIP(1, "Adherence", ip_values);

            m_Human.GetProperties()->Add(IPKeyValue("Adherence:High"));

            m_Human.SetHasHIV(true);
            m_Human.SetHIVInterventionsContainer(&m_InterventionsContainer);
            m_Human.SetHIVSusceptibility(&m_HIVSusceptibility);

            json::Object fakeConfigJson;
            Configuration * fakeConfigValid = Environment::CopyFromElement(fakeConfigJson);
            EventTriggerFactory::GetInstance()->Configure(fakeConfigValid);
            m_NEC.Initialize();
        }

        ~InterventionFixture()
        {
            delete m_pSimulationConfig;
            IPFactory::DeleteFactory();
            EventTriggerFactory::DeleteInstance();
            Environment::setSimulationConfig(nullptr);
            Environment::Finalize();
        }
    };

    void TestHelper_ConfigureException(int lineNumber, const std::string& rFilename, const std::string& rExpMsg)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile(rFilename.c_str()));

        ARTMortalityTable amt;
        try
        {
            bool ret = amt.Configure(p_config.get());
            CHECK(ret);

            CHECK_LN(false, lineNumber); // should not get here
        }
        catch (DetailedException& re)
        {
            std::string msg = re.GetMsg();
            if (msg.find(rExpMsg) == string::npos)
            {
                PrintDebug(rExpMsg);
                PrintDebug(msg);
                CHECK_LN(false, lineNumber);
            }
        }
    }

    TEST_FIXTURE(InterventionFixture, WrongTableDimensions)
    {
        TestHelper_ConfigureException(__LINE__, "testdata/ARTMortalityTableTest/WrongTableDimensions.json",
            "MortalityTable dimensions do not match provided bins");
    }

    TEST_FIXTURE(InterventionFixture, DocExample)
    {
        std::unique_ptr<Configuration> p_config(Configuration_Load("testdata/ARTMortalityTableTest/DocExample.json"));
        m_ARTTable.Configure(p_config.get());
        ICampaignCostObserverFake cco_fake;

        // ART duration low
        m_InterventionsContainer.SetDurationSinceLastStartingArt(1095);
        m_Human.SetAge(15 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(300);
        m_Human.SetRandUL( 0x80000000 ); // 0.5

        bool distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(1011, m_Human.GetDiseaseTimer(), 10);
        m_RealInterventionsContainer.GoOffART();
    }

    TEST_FIXTURE(InterventionFixture, InTablePoints)
    {
        std::unique_ptr<Configuration> p_config(Configuration_Load("testdata/ARTMortalityTableTest/Test.json"));
        m_ARTTable.Configure(p_config.get());
        ICampaignCostObserverFake cco_fake;

        // Check a few duration points on the table
        // Point 1
        m_Human.SetAge(50 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(500);

        bool distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(1.23808e+06, m_Human.GetDiseaseTimer(), 10);
        m_RealInterventionsContainer.GoOffART();

        // Point 2
        m_Human.SetAge(30 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(22);

        distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(431035, m_Human.GetDiseaseTimer(), 10);
        m_RealInterventionsContainer.GoOffART();
        CHECK_EQUAL(-1, m_RealInterventionsContainer.GetDurationSinceLastStartingART());
    }

    TEST_FIXTURE(InterventionFixture, OutOfTablePoints)
    {
        std::unique_ptr<Configuration> p_config(Configuration_Load("testdata/ARTMortalityTableTest/Test.json"));
        m_ARTTable.Configure(p_config.get());
        ICampaignCostObserverFake cco_fake;

        // ART duration low
        m_InterventionsContainer.SetDurationSinceLastStartingArt(0);
        m_Human.SetAge(30 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(150);

        bool distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(787923, m_Human.GetDiseaseTimer(), 10);
        m_RealInterventionsContainer.GoOffART();

        // ART duration high
        m_InterventionsContainer.SetDurationSinceLastStartingArt(1200);
        m_Human.SetAge(30 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(150);

        distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(787923, m_Human.GetDiseaseTimer(), 10);
        m_RealInterventionsContainer.GoOffART();

        // Age low
        m_InterventionsContainer.SetDurationSinceLastStartingArt(400);
        m_Human.SetAge(2 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(150);

        distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(883429, m_Human.GetDiseaseTimer(), 10);
        m_RealInterventionsContainer.GoOffART();

        // Age high
        m_InterventionsContainer.SetDurationSinceLastStartingArt(400);
        m_Human.SetAge(70 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(150);

        distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(613764, m_Human.GetDiseaseTimer(), 10);
        m_RealInterventionsContainer.GoOffART();

        // CD4 count low
        m_InterventionsContainer.SetDurationSinceLastStartingArt(400);
        m_Human.SetAge(30 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(0);

        distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(431035, m_Human.GetDiseaseTimer(), 10);
        m_RealInterventionsContainer.GoOffART();

        // CD4 count high
        m_InterventionsContainer.SetDurationSinceLastStartingArt(400);
        m_Human.SetAge(30 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(800);

        distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(1.57269e+06, m_Human.GetDiseaseTimer(), 10);
        m_RealInterventionsContainer.GoOffART();
    }

    TEST_FIXTURE(InterventionFixture, AdherentToNonAdherent)
    {
        std::unique_ptr<Configuration> p_config(Configuration_Load("testdata/ARTMortalityTableTest/Test.json"));
        m_ARTTable.Configure(p_config.get());
        ICampaignCostObserverFake cco_fake;

        m_InterventionsContainer.SetDurationSinceLastStartingArt(30);
        m_Human.SetAge(50 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(500);

        bool distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(1.23808e+06, m_Human.GetDiseaseTimer(), 10);

        distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);
        CHECK(distributed);
        CHECK_CLOSE(1.23808e+06, m_Human.GetDiseaseTimer(), 10);
    }

    TEST_FIXTURE(InterventionFixture, NonAdherentToAdherent)
    {
        std::unique_ptr<Configuration> p_config(Configuration_Load("testdata/ARTMortalityTableTest/Test.json"));
        m_ARTTable.Configure(p_config.get());
        ICampaignCostObserverFake cco_fake;

        m_InterventionsContainer.SetDurationSinceLastStartingArt(30);
        m_Human.SetAge(50 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(500);

        bool distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(1.23808e+06, m_Human.GetDiseaseTimer(), 10);

        m_Human.GetProperties()->Set(IPKeyValue("Adherence:High"));
        std::unique_ptr<Configuration> p_config2(Configuration_Load("testdata/ARTMortalityTableTest/Test2.json"));
        ARTMortalityTable ARTTable2;
        ARTTable2.Configure(p_config2.get());

        distributed = ARTTable2.Distribute(&m_InterventionsContext, &cco_fake);
        CHECK(distributed);
        CHECK_CLOSE(1.38547e+06, m_Human.GetDiseaseTimer(), 10);
    }

    TEST_FIXTURE(InterventionFixture, AdherentToAdherent)
    {
        std::unique_ptr<Configuration> p_config(Configuration_Load("testdata/ARTMortalityTableTest/Test.json"));
        m_ARTTable.Configure(p_config.get());
        ICampaignCostObserverFake cco_fake;

        m_InterventionsContainer.SetDurationSinceLastStartingArt(30);
        m_Human.SetAge(50 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(500);

        bool distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(1.23808e+06, m_Human.GetDiseaseTimer(), 10);

        m_InterventionsContainer.SetDurationSinceLastStartingArt(30);
        m_Human.SetAge(20 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(100);

        distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);
        CHECK(distributed);
        CHECK_CLOSE(766663, m_Human.GetDiseaseTimer(), 10);
    }

    TEST_FIXTURE(InterventionFixture, NonAdherentToNonAdherent)
    {
        std::unique_ptr<Configuration> p_config(Configuration_Load("testdata/ARTMortalityTableTest/Test.json"));
        m_ARTTable.Configure(p_config.get());
        ICampaignCostObserverFake cco_fake;

        m_InterventionsContainer.SetDurationSinceLastStartingArt(30);
        m_Human.SetAge(50 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(500);

        bool distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        CHECK(distributed);
        CHECK_CLOSE(1.23808e+06, m_Human.GetDiseaseTimer(), 10);

        m_InterventionsContainer.SetDurationSinceLastStartingArt(30);
        m_Human.SetAge(20 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(100);

        distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);
        CHECK(distributed);
        CHECK_CLOSE(766663, m_Human.GetDiseaseTimer(), 10);
        //CHECK(!m_fakeLogger.Empty());
        //LogEntry entry = m_fakeLogger.Back();
        //CHECK(entry.log_level == Logger::tLevel::WARNING);
        //CHECK(entry.msg.find("Start_Year (2003.000000) specified before Base_Year (2050.000000), for relationship type TRANSITORY\n") != string::npos);

        // TODO: it would be great to check for the presence of the "no change made" warning
        // TODO: it would be great to check for no event fired (and when events are fired above)
        // TODO: it would be great to add checks for time until suppression, too
        // TODO: and time-on-ART
    }

    TEST_FIXTURE(InterventionFixture, ZeroRate)
    {
        std::unique_ptr<Configuration> p_config(Configuration_Load("testdata/ARTMortalityTableTest/ZeroRate.json"));
        m_ARTTable.Configure(p_config.get());
        ICampaignCostObserverFake cco_fake;

        m_Human.SetAge(50 * DAYSPERYEAR);
        m_HIVSusceptibility.SetCD4Count(500);

        bool distributed = m_ARTTable.Distribute(&m_InterventionsContext, &cco_fake);

        // Zeros for rates in the mortality table should result in extremely long days-until-death
        CHECK(distributed);
        CHECK_CLOSE(3.40282347e+38, m_Human.GetDiseaseTimer(), 1.0E30);
    }

    TEST_FIXTURE(InterventionFixture, EmptyBins)
    {
        TestHelper_ConfigureException(__LINE__, "testdata/ARTMortalityTableTest/EmptyBins.json",
            "Bins for indexing into 'MortalityTable' field of 'ARTMortalityTable' intervention must not be empty.");
    }
}
