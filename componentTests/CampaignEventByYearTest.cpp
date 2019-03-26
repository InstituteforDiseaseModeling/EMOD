/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "SimulationConfig.h"
#include "CampaignEventByYear.h"
#include "IdmMpi.h"
#include "EventTrigger.h"
#include "Simulation.h"
#include "FakeLogger.h"
#include "componentTests.h"
#include <memory> // unique_ptr

using namespace std; 
using namespace Kernel; 

SUITE(CampaignEventByYearTest)
{
    static int m_NextId = 1 ;

    struct CampaignEventByYearFixture
    {
        IdmMpi::MessageInterface* m_pMpi;
        SimulationConfig* m_pSimulationConfig ;
        float m_oldBaseYear;

        CampaignEventByYearFixture()
            : m_pSimulationConfig( new SimulationConfig() )
        {
            JsonConfigurable::ClearMissingParameters();
            Environment::Finalize();
            Environment::setLogger( new FakeLogger( Logger::tLevel::WARNING ) );

            m_pMpi = IdmMpi::MessageInterface::CreateNull();

            int argc      = 1;
            char* exeName = "componentTests.exe";
            char** argv   = &exeName;
            string configFilename("testdata/CampaignEventByYearTest/config.json");
            string inputPath("testdata/CampaignEventByYearTest");
            string outputPath("testdata/CampaignEventByYearTest/output");
            string statePath("testdata/CampaignEventByYearTest");
            string dllPath("testdata/CampaignEventByYearTest");

            Environment::Initialize( m_pMpi, configFilename, inputPath, outputPath, /*statePath, */dllPath, false);

            m_pSimulationConfig->sim_type = SimType::HIV_SIM;

            Environment::setSimulationConfig( m_pSimulationConfig );

            EventTriggerFactory::DeleteInstance();
            EventTriggerFactory::GetInstance()->Configure( EnvPtr->Config );

            m_oldBaseYear = Simulation::base_year; // Need to save old base_year for restoration
        }

        ~CampaignEventByYearFixture()
        {
            EventTriggerFactory::DeleteInstance();
            Environment::Finalize();
            Simulation::base_year = m_oldBaseYear; // Restore base_year
        }
    };

    TEST_FIXTURE(CampaignEventByYearFixture, TestWarningBaseYearStartYear)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/CampaignEventByYearTest/TestSample.json"));
        FakeLogger fakeLogger(Logger::tLevel::WARNING);
        Environment::setLogger(&fakeLogger);

        Simulation::base_year = 2050;

        Array events = (*p_config)["Events"].As<Array>();
        unique_ptr<Configuration> event_config(Configuration::CopyFromElement(events[0], p_config->GetDataLocation()));

        try
        {
            CampaignEventByYear campaign_event_year;
            bool ret = campaign_event_year.Configure(event_config.get());
            CHECK(ret);
        }
        catch (DetailedException& de)
        {
            PrintDebug(de.GetMsg());
            CHECK(false);
        }

        CHECK(!fakeLogger.Empty());
        LogEntry entry = fakeLogger.Back();
        CHECK(entry.log_level == Logger::tLevel::WARNING);
        CHECK(entry.msg.find("Start_Year (2003.000000) specified before Base_Year (2050.000000)\n") != string::npos);
    }
}
