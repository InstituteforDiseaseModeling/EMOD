/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>
#include "componentTests.h"

#include "AdherentDrug.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "ICampaignCostObserverFake.h"
#include "Configuration.h"
#include "Simulation.h"
#include "SimulationConfig.h"
#include "MalariaParameters.h"
#include "MalariaDrugTypeParameters.h"
#include "GenomeMarkers.h"
#include "StrainIdentity.h"

using namespace Kernel;

SUITE( AdherentDrugTest )
{
    struct AdherentDrugFixture
    {
        INodeContextFake                        m_NC;
        INodeEventContextFake                   m_NEC;
        IndividualHumanInterventionsContextFake m_InterventionsContext;
        IndividualHumanContextFake              m_Human;
        AdherentDrug                            m_Drug;
        SimulationConfig*                       m_pSimulationConfig;
        GenomeMarkers                           m_GenomeMarkers;

        AdherentDrugFixture()
            : m_NC()
            , m_NEC()
            , m_InterventionsContext()
            , m_Human( &m_InterventionsContext, &m_NC, &m_NEC, nullptr )
            , m_Drug()
            , m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            std::unique_ptr<Configuration> p_drug_config( Configuration_Load( "testdata/AdherentDrugTest/Drug.json" ) );
            MalariaDrugTypeParameters* pmd = MalariaDrugTypeParameters::CreateMalariaDrugTypeParameters( p_drug_config.get(), "TestDrug", m_GenomeMarkers );
            m_pSimulationConfig->malaria_params->MalariaDrugMap[ "TestDrug" ] = pmd;

            // --------------------------------------------------------------------------
            // --- We need CONCENTRATION_VERSUS_TIME so we can see efficacy degrade and
            // --- get increase as different doses are taken
            // --------------------------------------------------------------------------
            m_pSimulationConfig->malaria_params->PKPD_model = PKPDModel::CONCENTRATION_VERSUS_TIME;

            m_InterventionsContext.SetContextTo( &m_Human );

            IPFactory::DeleteFactory();
            EventTriggerFactory::DeleteInstance();

            json::Object fakeConfigJson;
            Configuration * fakeConfigValid = Environment::CopyFromElement( fakeConfigJson );
            EventTriggerFactory::GetInstance()->Configure( fakeConfigValid );

            m_NEC.Initialize();
        }

        ~AdherentDrugFixture()
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

                bool ret = m_Drug.Configure( p_config.get() );
                CHECK( ret );

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

    TEST_FIXTURE( AdherentDrugFixture, TestCountAndEntryMismatch )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AdherentDrugTest/TestCountAndEntryMismatch.json",
                                       "'Adherence_Config' is not configured correctly.\n'Drug_Type'=TestDrug is configured for 3 dose(s)\nbut the IWaningEffectCount does not support that number of doses.\nThere should probably be one entry for each dose." );
    }

    TEST_FIXTURE( AdherentDrugFixture, TestCountAndNumDoseMismatch )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AdherentDrugTest/TestCountAndNumDoseMismatch.json",
                                       "'Adherence_Config' is not configured correctly.\n'Drug_Type'=TestDrug is configured for 3 dose(s)\nbut the IWaningEffectCount does not support that number of doses.\nThere should probably be one entry for each dose." );
    }

    TEST_FIXTURE( AdherentDrugFixture, TestMissingDrugType )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AdherentDrugTest/TestMissingDrugType.json",
                                       "'Drug_Type' was not defined and it is a required parameter." );
    }

    TEST_FIXTURE( AdherentDrugFixture, TestEmptyAdherenceConfig )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AdherentDrugTest/TestEmptyAdherenceConfig.json",
                                       "The Adherence_Config must be defined with a valid WaningEffect." );
    }

    TEST_FIXTURE( AdherentDrugFixture, TestNonAdherenceOptionsUnknown )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AdherentDrugTest/TestNonAdherenceOptionsUnknown.json",
                                       "Failed to find enum match for value XXX and key Non_Adherence_Options" );
    }

    TEST_FIXTURE( AdherentDrugFixture, TestNonAdherenceOptionsDuplicate )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AdherentDrugTest/TestNonAdherenceOptionsDuplicate.json",
                                       "'Non_Adherence_Options' has duplicate entries.  It can have at most one of each enum.\n'Non_Adherence_Options' values are: STOP, STOP" );
    }

    TEST_FIXTURE( AdherentDrugFixture, TestNonAdherenceOptionsDistributionSize )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AdherentDrugTest/TestNonAdherenceOptionsDistributionSize.json",
                                       "Variable or parameter 'Non_Adherence_Options.<num elements>' with value 1 is incompatible with variable or parameter 'Non_Adherence_Distribution.<num elements>' with value 3. These two arrays must have the same number of elements." );
    }

    TEST_FIXTURE( AdherentDrugFixture, TestNonAdherenceDistributionSum )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/AdherentDrugTest/TestNonAdherenceDistributionSum.json",
                                       "Variable or parameter 'Sum(Non_Adherence_Distribution)' with value 1.5 is incompatible with variable or parameter 'Required' with value 1. The values of 'Non_Adherence_Distribution' must sum to 1.0." );
    }

    TEST_FIXTURE( AdherentDrugFixture, TestNextUpdate )
    {
        StrainIdentity strain;

        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/AdherentDrugTest/TestNextUpdate.json" ) );
        m_Drug.Configure( p_config.get() );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );

        bool distributed = m_Drug.Distribute( &m_InterventionsContext, nullptr );
        CHECK( distributed );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.00001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the first dose
        // --- on the first update. - Take Dose is false - Try again NEXT_UPDATE
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the first dose
        // --- on the second update. - Time and Take Dose is True
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.857, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 2.957, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.086, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.171, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.257, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.343, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- on the third update. - Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.729, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 2.514, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.073, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.146, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.219, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.292, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- on the fourth update. - Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.572, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 1.974, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.057, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.114, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.172, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.229, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the second dose
        // --- on the fifth update. - Time to take and Takes Dose
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.871, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 3.004, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.087, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.174, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.261, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.348, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the third dose
        // --- on the sixth update. - Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.759, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the third dose
        // --- on the seventh update. - Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.623, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the third dose
        // --- on the eigth update. - Take Dose is False - Try again NEXT_UPDATE
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.494, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the third dose
        // --- on the ninth update. - Take Dose is True
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.869, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person has no more doses by watching
        // --- the efficacy degrade over time
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.759, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.626, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        float prev = m_Drug.GetDrugCurrentEfficacy();
        for( int i = 0 ; i < 20 ; ++i )
        {
            m_Drug.Update( 1.0 );
            float current = m_Drug.GetDrugCurrentEfficacy();
            CHECK( prev > current );
            prev = current;
        }
    }

    TEST_FIXTURE( AdherentDrugFixture, TestNextDosageTime )
    {
        StrainIdentity strain;

        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/AdherentDrugTest/TestNextDosageTime.json" ) );
        m_Drug.Configure( p_config.get() );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );

        bool distributed = m_Drug.Distribute( &m_InterventionsContext, nullptr );
        CHECK( distributed );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.00001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the first dose
        // --- Take Dose is false - Try again NEXT_DOSAGE_TIME
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the first dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the first dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.0, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the first dose
        // --- Take Dose is True
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.857, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 2.957, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.086, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.171, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.257, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.343, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not time (dose interval
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.729, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.572, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the second dose
        // --- Take Dose is True
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.871, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the third dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.759, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the third dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.623, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the third dose
        // --- Take Dose is FALSE - NEXT_DOSAGE_TIME
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.494, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the third dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.391, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the third dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.313, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the third dose
        // --- Take Dose is True
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.864, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person has no more doses by watching
        // --- the efficacy degrade over time
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.747, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.605, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        float prev = m_Drug.GetDrugCurrentEfficacy();
        for( int i = 0; i < 20; ++i )
        {
            m_Drug.Update( 1.0 );
            float current = m_Drug.GetDrugCurrentEfficacy();
            CHECK( prev > current );
            prev = current;
        }
    }

    TEST_FIXTURE( AdherentDrugFixture, TestLostTakeNext )
    {
        StrainIdentity strain;

        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/AdherentDrugTest/TestLostTakeNext.json" ) );
        m_Drug.Configure( p_config.get() );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );

        bool distributed = m_Drug.Distribute( &m_InterventionsContext, nullptr );
        CHECK( distributed );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.00001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the first dose
        // --- Take Dose is True but LOST_TAKE_NEXT
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.857, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 2.957, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.086, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.171, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.257, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.343, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.729, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 2.514, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.073, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.146, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.219, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.292, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.572, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 1.974, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.057, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.114, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.172, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.229, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the second dose
        // --- Take Dose is True
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.871, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 3.004, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.087, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.174, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.261, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.348, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Since the person lost a dose, they only get two doses.
        // --- The effect should just degrade over time.
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.759, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        float prev = m_Drug.GetDrugCurrentEfficacy();
        for( int i = 0; i < 20; ++i )
        {
            m_Drug.Update( 1.0 );
            float current = m_Drug.GetDrugCurrentEfficacy();
            CHECK( prev > current );
            prev = current;
        }
    }

    TEST_FIXTURE( AdherentDrugFixture, TestStop )
    {
        StrainIdentity strain;

        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/AdherentDrugTest/TestStop.json" ) );
        m_Drug.Configure( p_config.get() );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );

        bool distributed = m_Drug.Distribute( &m_InterventionsContext, nullptr );
        CHECK( distributed );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.00001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the first dose
        // --- Take Dose is True
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.857, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 2.957, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.086, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.171, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.257, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.343, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.729, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 2.514, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.073, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.146, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.219, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.292, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.572, m_Drug.GetDrugCurrentEfficacy(), 0.001 );
        CHECK_CLOSE( 1.974, m_Drug.get_drug_IRBC_killrate( strain ), 0.001 );
        CHECK_CLOSE( 0.057, m_Drug.get_drug_gametocyte02(  strain ), 0.001 );
        CHECK_CLOSE( 0.114, m_Drug.get_drug_gametocyte34(  strain ), 0.001 );
        CHECK_CLOSE( 0.172, m_Drug.get_drug_gametocyteM(   strain ), 0.001 );
        CHECK_CLOSE( 0.229, m_Drug.get_drug_hepatocyte(    strain ), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Take Dose is False - STOP
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK( m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.428, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Since the person stop taking the drug, we see the
        // --- efficacy degrace over time.
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.319, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        float prev = m_Drug.GetDrugCurrentEfficacy();
        for( int i = 0; i < 20; ++i )
        {
            m_Drug.Update( 1.0 );
            float current = m_Drug.GetDrugCurrentEfficacy();
            CHECK( prev > current );
            prev = current;
        }
    }

    TEST_FIXTURE( AdherentDrugFixture, TestDosageCount )
    {
        StrainIdentity strain;

        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/AdherentDrugTest/TestDosageCount.json" ) );
        m_Drug.Configure( p_config.get() );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );

        bool distributed = m_Drug.Distribute( &m_InterventionsContext, nullptr );
        CHECK( distributed );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.00001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the first dose
        // --- First dose probability is 0.9 and Random number is 0.9375
        // --- Try again NEXT_UPDATE
        // ------------------------------------------------------
        m_Human.SetRandUL( 0xF0000000 ); // 0.9375
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the first dose
        // --- First dose probability is 0.9 and Random number is 0.5
        // --- Takes dose
        // ------------------------------------------------------
        m_Human.SetRandUL( 0x80000000 ); // 0.5
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.857, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.729, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.572, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Second dose probability is 0.1 and Random number is 0.5
        // --- Try again NEXT_UPDATE
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.428, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Second dose probability is 0.1 and Random number is 0.5
        // --- Try again NEXT_UPDATE
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.319, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the second dose
        // --- Second dose probability is 0.1 and Random number is 0.0625
        // --- Takes dose
        // ------------------------------------------------------
        m_Human.SetRandUL( 0x10000000 ); // 0.0625
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.864, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the third dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.745, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the third dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.602, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the third dose
        // --- Third dose probability is 1.0 and Random number is 0.0625
        // --- Takes dose
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.873, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person has no more doses by watching
        // --- the efficacy degrade over time
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.765, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        float prev = m_Drug.GetDrugCurrentEfficacy();
        for( int i = 0; i < 20; ++i )
        {
            m_Drug.Update( 1.0 );
            float current = m_Drug.GetDrugCurrentEfficacy();
            CHECK( prev > current );
            prev = current;
        }
    }

    TEST_FIXTURE( AdherentDrugFixture, TestMaxDuration )
    {
        StrainIdentity strain;

        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/AdherentDrugTest/TestMaxDuration.json" ) );
        m_Drug.Configure( p_config.get() );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );

        bool distributed = m_Drug.Distribute( &m_InterventionsContext, nullptr );
        CHECK( distributed );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.00001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the first dose
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK_CLOSE( 0.857, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.729, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.572, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Second dose probability is 0.0
        // --- Try again NEXT_DOSAGE_TIME
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.428, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.319, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.244, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Max_Dose_Consideration_Duration is exceeded so STOP
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.190, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        float prev = m_Drug.GetDrugCurrentEfficacy();
        for( int i = 0; i < 20; ++i )
        {
            m_Drug.Update( 1.0 );
            float current = m_Drug.GetDrugCurrentEfficacy();
            CHECK( prev > current );
            prev = current;
        }
    }

    TEST_FIXTURE( AdherentDrugFixture, TestOptionSelection )
    {
        StrainIdentity strain;

        std::unique_ptr<Configuration> p_drug_config( Configuration_Load( "testdata/AdherentDrugTest/10_Dose_Drug.json" ) );
        MalariaDrugTypeParameters* pmd = MalariaDrugTypeParameters::CreateMalariaDrugTypeParameters( p_drug_config.get(), "10_Dose_Drug", m_GenomeMarkers );
        m_pSimulationConfig->malaria_params->MalariaDrugMap[ "10_Dose_Drug" ] = pmd;


        std::unique_ptr<Configuration> p_config( Configuration_Load( "testdata/AdherentDrugTest/TestOptionSelection.json" ) );
        m_Drug.Configure( p_config.get() );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );

        bool distributed = m_Drug.Distribute( &m_InterventionsContext, nullptr );
        CHECK( distributed );

        CHECK( !m_Drug.Expired() );
        CHECK( !m_InterventionsContext.HasDrugEffects() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.00001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the first dose
        // --- Probability of first dose is 0.5 and Random number is 0.9375
        // --- Since NEXT_UPDATE is first in the list, its probability
        // --- is 0.4, and the random number is 0.0625, try again NEXT_UPDATE
        // ------------------------------------------------------
        std::vector<uint32_t> random_uls;
        random_uls.push_back( 0xF0000000 ); // 0.9375
        random_uls.push_back( 0x10000000 ); // 0.0625
        m_Human.SetRandUL( random_uls );
        m_Drug.Update( 1.0 );
        CHECK( !m_Drug.Expired() );
        CHECK_CLOSE( 0.0, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the first dose
        // --- Probability is 0.5 and Random number is 0.625
        // ------------------------------------------------------
        random_uls.clear();
        random_uls.push_back( 0x10000000 ); // 0.0625
        random_uls.push_back( 0x10000000 ); // 0.0625
        m_Human.SetRandUL( random_uls );
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.857, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.729, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.572, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Second dose probability is 0.5 and Ranodm number is 0.9375
        // --- Since NEXT_DOSAGE_TIME is second in the list, its probability
        // --- is 0.3, and the random number is 0.5, try again NEXT_DOSAGE_TIME
        // ------------------------------------------------------
        random_uls.clear();
        random_uls.push_back( 0xF0000000 ); // 0.9375
        random_uls.push_back( 0x80000000 ); // 0.5
        m_Human.SetRandUL( random_uls );
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.428, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.319, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the second dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.244, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the second dose
        // --- Second dose probability is 0.5 and Random number is 0.0625
        // ------------------------------------------------------
        random_uls.clear();
        random_uls.push_back( 0x10000000 ); // 0.0625
        random_uls.push_back( 0x80000000 ); // 0.5
        m_Human.SetRandUL( random_uls );
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.862, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the third dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.741, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the third dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.595, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person DOES take the thrid dose
        // --- Third dose probability is 0.5 and Ranodm number is 0.9375
        // --- BUT since LOST_TAKE_NEXT is third in the list, its probability
        // --- is 0.2, and the random number is 0.75, LOST_TAKE_NEXT
        // ------------------------------------------------------
        random_uls.clear();
        random_uls.push_back( 0xF0000000 ); // 0.9375
        random_uls.push_back( 0xC0000000 ); // 0.75
        m_Human.SetRandUL( random_uls );
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.872, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the forth dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.763, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the forth dose
        // --- Not Time (dose interval)
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.632, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take the forth dose
        // --- Forth dose probability is 0.5 and Ranodm number is 0.9375
        // --- Since STOP is forth in the list, its probability
        // --- is 0.1, and the random number is 0.9375, STOP is
        // --- STOP is selected and we take no more doses.
        // ------------------------------------------------------
        random_uls.clear();
        random_uls.push_back( 0xF0000000 ); // 0.9375
        random_uls.push_back( 0xF0000000 ); // 0.9375
        m_Human.SetRandUL( random_uls );
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.507, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        // ------------------------------------------------------
        // --- Test that the person does NOT take any more doses
        // ------------------------------------------------------
        m_Drug.Update( 1.0 );
        CHECK_CLOSE( 0.405, m_Drug.GetDrugCurrentEfficacy(), 0.001 );

        float prev = m_Drug.GetDrugCurrentEfficacy();
        for( int i = 0; i < 20; ++i )
        {
            m_Drug.Update( 1.0 );
            float current = m_Drug.GetDrugCurrentEfficacy();
            CHECK( prev > current );
            prev = current;
        }
    }
}
