/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"
#include "NChooserEventCoordinatorHIV.h"
#include "Node.h"
#include "SimulationConfig.h"

#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "IdmMpi.h"
#include "EventTrigger.h"
#include "Simulation.h"
#include "FakeLogger.h"

using namespace std; 
using namespace Kernel; 

SUITE(NChooserEventCoordinatorTest)
{
    static int m_NextId = 1 ;

    struct NChooserEventCoordinatorFixture
    {
        IdmMpi::MessageInterface* m_pMpi;
        SimulationConfig* m_pSimulationConfig ;
        float m_oldBaseYear;
        RANDOMBASE* m_pRNG;

        NChooserEventCoordinatorFixture()
            : m_pSimulationConfig( new SimulationConfig() )
        {
            JsonConfigurable::ClearMissingParameters();
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );

            m_pMpi = IdmMpi::MessageInterface::CreateNull();

            int argc      = 1;
            char* exeName = "componentTests.exe";
            char** argv   = &exeName;
            string configFilename("testdata/NChooserEventCoordinatorTest/config.json");
            string inputPath("testdata/NChooserEventCoordinatorTest");
            string outputPath("testdata/NChooserEventCoordinatorTest/output");
            string statePath("testdata/NChooserEventCoordinatorTest");
            string dllPath("testdata/NChooserEventCoordinatorTest");

            Environment::Initialize( m_pMpi, configFilename, inputPath, outputPath, /*statePath, */dllPath, false);

            m_pRNG = new PSEUDO_DES(0);

            m_pSimulationConfig->sim_type = SimType::HIV_SIM;

            Environment::setSimulationConfig( m_pSimulationConfig );
            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();

            std::map<std::string,float> location_ip_values ;
            location_ip_values.insert( std::make_pair( "URBAN", 0.8f ) );
            location_ip_values.insert( std::make_pair( "RURAL", 0.2f ) );

            IPFactory::GetInstance()->AddIP( 1, "Location", location_ip_values );

            std::map<std::string,float> income_ip_values ;
            income_ip_values.insert( std::make_pair( "LOW",  0.9f ) );
            income_ip_values.insert( std::make_pair( "HIGH", 0.1f ) );

           IPFactory::GetInstance()->AddIP( 1, "Income", income_ip_values );

            EventTriggerFactory::DeleteInstance();
            EventTriggerFactory::GetInstance()->Configure( EnvPtr->Config );

            m_oldBaseYear = Simulation::base_year; // Need to save old base_year for restoration
        }

        ~NChooserEventCoordinatorFixture()
        {
            EventTriggerFactory::DeleteInstance();
            IPFactory::DeleteFactory();
            Environment::Finalize();
            Simulation::base_year = m_oldBaseYear; // Restore base_year
            delete m_pRNG;
        }
    };

    IIndividualHumanContext* CreateHuman( 
        INodeContext* pnc,
        INodeEventContext* pnec,
        int gender, 
        float ageDays, 
        bool hasHIV, 
        bool hasTestedPositiveForHIV,
        bool isCircumcised,
        const std::string& rPropertyName = std::string(), 
        const std::string& rPropertyValue = std::string() )
    {
        IndividualHumanInterventionsContextFake* p_hic = new IndividualHumanInterventionsContextFake();
        IndividualHumanContextFake* p_human = new IndividualHumanContextFake( p_hic, pnc, pnec, nullptr );

        if( hasTestedPositiveForHIV )
        {
            release_assert( hasHIV );
        }
        p_human->SetId( m_NextId++ );
        p_human->SetGender( gender );
        p_human->SetAge( ageDays );
        p_human->SetHasHIV( hasHIV );
        p_human->SetIsCircumcised( isCircumcised );
        p_hic->OnTestForHIV( hasTestedPositiveForHIV );
        p_human->GetProperties()->Add( IPKeyValue( rPropertyName, rPropertyValue ) );

        return (IIndividualHumanContext*)p_human ;
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestTargetedByAgeAndGenderNumTargeted)
    {
        TargetedByAgeAndGender ag1( AgeRange( 15.0, 30.0 ), Gender::COUNT, 100, 1, 0 );
        CHECK_EQUAL( 100, ag1.GetNumTargeted() );

        TargetedByAgeAndGender ag2( AgeRange( 15.0, 30.0 ), Gender::COUNT, 100, 6, 0 );
        CHECK_EQUAL( 17, ag2.GetNumTargeted() );
        ag2.IncrementNextNumTargets();
        CHECK_EQUAL( 17, ag2.GetNumTargeted() );
        ag2.IncrementNextNumTargets();
        CHECK_EQUAL( 17, ag2.GetNumTargeted() );
        ag2.IncrementNextNumTargets();
        CHECK_EQUAL( 17, ag2.GetNumTargeted() );
        ag2.IncrementNextNumTargets();
        CHECK_EQUAL( 16, ag2.GetNumTargeted() );
        ag2.IncrementNextNumTargets();
        CHECK_EQUAL( 16, ag2.GetNumTargeted() );

        TargetedByAgeAndGender ag3( AgeRange( 15.0, 30.0 ), Gender::COUNT, 100, 6, 3 );
        CHECK_EQUAL( 17, ag3.GetNumTargeted() );
        ag3.IncrementNextNumTargets();
        CHECK_EQUAL( 16, ag3.GetNumTargeted() );
        ag3.IncrementNextNumTargets();
        CHECK_EQUAL( 16, ag3.GetNumTargeted() );

        TargetedByAgeAndGender ag4( AgeRange( 15.0, 30.0 ), Gender::MALE, 6, 2, 0 );
        CHECK_EQUAL( 3, ag4.GetNumTargeted() );
        ag4.IncrementNextNumTargets();
        CHECK_EQUAL( 3, ag4.GetNumTargeted() );

        TargetedByAgeAndGender ag5( AgeRange( 15.0, 30.0 ), Gender::MALE, 500, 182, 0 );
        for( int i = 0 ; i < 136 ; ++i )
        {
            CHECK_EQUAL( 3, ag5.GetNumTargeted() );
            ag5.IncrementNextNumTargets();
        }
        for( int i = 136 ; i < 182 ; ++i )
        {
            CHECK_EQUAL( 2, ag5.GetNumTargeted() );
            ag5.IncrementNextNumTargets();
        }
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestTargetedByAgeAndGender)
    {
        INodeContextFake nc;
        INodeEventContextFake nec;
        nec.Initialize();

        nec.Add( CreateHuman( &nc, &nec, Gender::FEMALE, 20*DAYSPERYEAR, false, false, false, "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,    1*DAYSPERYEAR, false, false, true,  "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,   30*DAYSPERYEAR, true,  true,  true,  "Location", "RURAL" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::FEMALE, 21*DAYSPERYEAR, false, false, false, "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::FEMALE, 22*DAYSPERYEAR, false, false, false, "Location", "RURAL" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,   23*DAYSPERYEAR, true,  false, false, "Location", "RURAL" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,   24*DAYSPERYEAR, true,  true,  false, "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::FEMALE, 25*DAYSPERYEAR, true,  false, false, "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,   26*DAYSPERYEAR, false, false, true,  "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::FEMALE, 27*DAYSPERYEAR, false, false, false, "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,   28*DAYSPERYEAR, false, false, false, "Location", "RURAL" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,   29*DAYSPERYEAR, true,  true,  false, "Location", "RURAL" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::FEMALE, 15*DAYSPERYEAR, true,  true,  false, "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::FEMALE, 16*DAYSPERYEAR, false, false, false, "Location", "RURAL" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::FEMALE, 17*DAYSPERYEAR, false, false, false, "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,   18*DAYSPERYEAR, true,  false, true,  "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,   19*DAYSPERYEAR, false, false, false, "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,  100*DAYSPERYEAR, false, false, false, "Location", "URBAN" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::FEMALE, 20*DAYSPERYEAR, true,  false, false, "Location", "RURAL" ) );
        nec.Add( CreateHuman( &nc, &nec, Gender::MALE,   20*DAYSPERYEAR, false, false, true,  "Location", "URBAN" ) );

        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/NChooserEventCoordinatorTest/TestTargetedByAgeAndGender.json" ) );
        PropertyRestrictions<IPKey, IPKeyValue, IPKeyValueContainer> pr;
        pr.ConfigureFromJsonAndKey( p_config.get(), "Property_Restrictions_Within_Node" );

        DiseaseQualifications disease_qual;

        TargetedByAgeAndGender ag1( AgeRange( 15.0, 30.0 ), Gender::COUNT, 10, 3, 0 );

        ag1.FindQualifyingIndividuals( &nec, disease_qual, pr );

        std::vector<IIndividualHumanEventContext*> selected_list_1 = ag1.SelectIndividuals( m_pRNG );
        CHECK_EQUAL(  4, selected_list_1.size() );
        CHECK_EQUAL(  1, selected_list_1[0]->GetSuid().data );
        CHECK_EQUAL(  7, selected_list_1[1]->GetSuid().data );
        CHECK_EQUAL( 13, selected_list_1[2]->GetSuid().data );
        CHECK_EQUAL( 15, selected_list_1[3]->GetSuid().data );

        ag1.IncrementNextNumTargets();

        ag1.FindQualifyingIndividuals( &nec, disease_qual, pr );

        std::vector<IIndividualHumanEventContext*> selected_list_2 = ag1.SelectIndividuals( m_pRNG );
        CHECK_EQUAL(  3, selected_list_2.size() );
        CHECK_EQUAL( 13, selected_list_2[0]->GetSuid().data );
        CHECK_EQUAL( 15, selected_list_2[1]->GetSuid().data );
        CHECK_EQUAL( 16, selected_list_2[2]->GetSuid().data );

        ag1.IncrementNextNumTargets();

        ag1.FindQualifyingIndividuals( &nec, disease_qual, pr );

        std::vector<IIndividualHumanEventContext*> selected_list_3 = ag1.SelectIndividuals( m_pRNG );
        CHECK_EQUAL(  3, selected_list_3.size() );
        CHECK_EQUAL( 10, selected_list_3[0]->GetSuid().data );
        CHECK_EQUAL( 13, selected_list_3[1]->GetSuid().data );
        CHECK_EQUAL( 16, selected_list_3[2]->GetSuid().data );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestSample)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/NChooserEventCoordinatorTest/TestSample.json" ) );

        NChooserEventCoordinatorHIV coordinator;

        try
        {
            bool ret = coordinator.Configure( p_config.get() );
            CHECK( ret );
        }
        catch( DetailedException& de )
        {
            PrintDebug( de.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestValidSimTypeGeneric)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/NChooserEventCoordinatorTest/TestValidSimTypeGeneric.json" ) );

        // -----------------------------------------------------------------
        // --- Test that generic version works with non-STI & HIV sim types
        // -----------------------------------------------------------------
        m_pSimulationConfig->sim_type = SimType::TBHIV_SIM;

        try
        {
            NChooserEventCoordinator generic_nchooser;
            bool ret = generic_nchooser.Configure( p_config.get() );
            CHECK( ret );
        }
        catch( DetailedException& de )
        {
            PrintDebug( de.GetMsg() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestValidSimTypeSTI)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/NChooserEventCoordinatorTest/TestValidSimTypeSTI.json" ) );

        // -----------------------------------------------------------------
        // --- Test that STI version works with STI sim
        // -----------------------------------------------------------------
        m_pSimulationConfig->sim_type = SimType::STI_SIM;

        try
        {
            NChooserEventCoordinatorSTI sti_nchooser;
            bool ret = sti_nchooser.Configure( p_config.get() );
            CHECK( ret );
        }
        catch( DetailedException& de )
        {
            PrintDebug( de.GetMsg() );
            CHECK( false );
        }

        // -----------------------------------------------------------------
        // --- Test that STI version works with HIV sim
        // -----------------------------------------------------------------
        m_pSimulationConfig->sim_type = SimType::HIV_SIM;

        try
        {
            NChooserEventCoordinatorSTI sti_nchooser;
            bool ret = sti_nchooser.Configure( p_config.get() );
            CHECK( ret );
        }
        catch( DetailedException& de )
        {
            PrintDebug( de.GetMsg() );
            CHECK( false );
        }

        // -----------------------------------------------------------------
        // --- Test that STI version does not work with GENERIC
        // -----------------------------------------------------------------
        m_pSimulationConfig->sim_type = SimType::GENERIC_SIM;

        try
        {
            NChooserEventCoordinatorSTI sti_nchooser;
            bool ret = sti_nchooser.Configure( p_config.get() );
            CHECK( false );
        }
        catch( DetailedException&  )
        {
            CHECK( true );
        }

        // -----------------------------------------------------------------
        // --- Test that STI version does not work with other sim type
        // -----------------------------------------------------------------
        m_pSimulationConfig->sim_type = SimType::TBHIV_SIM;

        try
        {
            NChooserEventCoordinatorSTI sti_nchooser;
            bool ret = sti_nchooser.Configure( p_config.get() );
            CHECK( false );
        }
        catch( DetailedException& )
        {
            CHECK( true );
        }
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestValidSimTypeHIV)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/NChooserEventCoordinatorTest/TestValidSimTypeHIV.json" ) );

        // -----------------------------------------------------------------
        // --- Test that HIV version works with HIV sim
        // -----------------------------------------------------------------
        m_pSimulationConfig->sim_type = SimType::HIV_SIM;

        try
        {
            NChooserEventCoordinatorHIV hiv_nchooser;
            bool ret = hiv_nchooser.Configure( p_config.get() );
            CHECK( ret );
        }
        catch( DetailedException& de )
        {
            PrintDebug( de.GetMsg() );
            CHECK( false );
        }

        // -----------------------------------------------------------------
        // --- Test that HIV version does not work with STI sim
        // -----------------------------------------------------------------
        m_pSimulationConfig->sim_type = SimType::STI_SIM;

        try
        {
            NChooserEventCoordinatorHIV hiv_nchooser;
            bool ret = hiv_nchooser.Configure( p_config.get() );
            CHECK( false );
        }
        catch( DetailedException& )
        {
            CHECK( true );
        }

        // -----------------------------------------------------------------
        // --- Test that HIV version does not work with GENERIC
        // -----------------------------------------------------------------
        m_pSimulationConfig->sim_type = SimType::GENERIC_SIM;

        try
        {
            NChooserEventCoordinatorHIV hiv_nchooser;
            bool ret = hiv_nchooser.Configure( p_config.get() );
            CHECK( false );
        }
        catch( DetailedException& )
        {
            CHECK( true );
        }

        // -----------------------------------------------------------------
        // --- Test that HIV version does not work with other sim type
        // -----------------------------------------------------------------
        m_pSimulationConfig->sim_type = SimType::TBHIV_SIM;

        try
        {
            NChooserEventCoordinatorHIV hiv_nchooser;
            bool ret = hiv_nchooser.Configure( p_config.get() );
            CHECK( false );
        }
        catch( DetailedException& )
        {
            CHECK( true );
        }
    }

    void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        try
        {
            std::map<std::string,float> ip_values ;
            ip_values.insert( std::make_pair( "HUMAN",    0.2f ) );
            ip_values.insert( std::make_pair( "VULCAN",   0.2f ) );
            ip_values.insert( std::make_pair( "KLINGON",  0.2f ) );
            ip_values.insert( std::make_pair( "ANDORIAN", 0.2f ) );
            ip_values.insert( std::make_pair( "ROMULAN",  0.2f ) );

            IPFactory::GetInstance()->AddIP( 1, "Race", ip_values );

            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

            NChooserEventCoordinatorHIV coordinator;

            bool ret = coordinator.Configure( p_config.get() );
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

#if 1
    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidYears)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidYears.json",
            "Variable or parameter 'Start_Year' with value 2100 is incompatible with variable or parameter 'End_Year' with value 1950. Start_Year must be < End_Year" );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestNoNumTargeted)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestNoNumTargeted.json",
            "The arrays 'Age_Range_Years', 'Num_Targeted_Males', and 'Num_Targeted_Females' have zero elements.\nNum_Targeted_Males, Num_Targeted_Females, and Age_Range_Years must have the same number of elements, but not zero.  There must be one age range for each number targeted." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidNumTargeted)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidNumTargeted.json",
            "The number of elements in 'Num_Targeted'(=4) is not the same as 'Age_Ranges_Years'(=1).\n'Num_Targeted' and 'Age_Range_Years' must have the same number of elements, but not zero.  There must be one age range for each number targeted." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidNumTargetedZeroValues)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidNumTargetedZeroValues.json",
            "'Num_Targeted' has zero values and won't target anyone." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidNumTargetedMalesFemalesZeroValues)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidNumTargetedMalesFemalesZeroValues.json",
            "'Num_Targeted_Males' and 'Num_Targeted_Females' have zero values and won't target anyone." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidNumTargetedMales)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidNumTargetedMales.json",
            "The number of elements in 'Num_Targeted_Males' is 1.\nThe number of elements in 'Num_Targeted_Females' is 2.\nThe number of elements in 'Age_Range_Years' is 2.\n'Num_Targeted_Males', 'Num_Targeted_Females', and 'Age_Range_Years' must have the same number of elements, but not zero.  There must be one age range for each number targeted." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidNumTargetedFemales)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidNumTargetedFemales.json",
            "The number of elements in 'Num_Targeted_Males' is 2.\nThe number of elements in 'Num_Targeted_Females' is 6.\nThe number of elements in 'Age_Range_Years' is 2.\n'Num_Targeted_Males', 'Num_Targeted_Females', and 'Age_Range_Years' must have the same number of elements, but not zero.  There must be one age range for each number targeted." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidNumAgeRanges)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidNumAgeRanges.json",
            "The number of elements in 'Num_Targeted_Males' is 2.\nThe number of elements in 'Num_Targeted_Females' is 2.\nThe number of elements in 'Age_Range_Years' is 1.\n'Num_Targeted_Males', 'Num_Targeted_Females', and 'Age_Range_Years' must have the same number of elements, but not zero.  There must be one age range for each number targeted." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidAgeRange)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidAgeRange.json",
            "Variable or parameter 'Min' with value 100 is incompatible with variable or parameter 'Max' with value 9. Min must be < Max" );
    }
#endif

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidDiseaseState)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidDiseaseState.json",
            "Constrained strings (dynamic enum) with specified value XXXXXXX invalid. Possible values are: HIV_Negative...HIV_Positive...Has_Intervention...Male_Circumcision_Negative...Male_Circumcision_Positive...Not_Have_Intervention...Tested_Negative...Tested_Positive..." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidPeriodOverlap)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidPeriodOverlap.json",
            "'Distributions' cannot have time periods that overlap.  (2000, 2010) vs (2005, 2015)" );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidAgeRangeOverlap)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidAgeRangeOverlap.json",
            "'Age_Ranges_Years' cannot have age ranges that overlap.  (30, 50) vs (40, 60)" );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestNoDistributions)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestNoDistributions.json",
            "'Distributions' cannot have zero elements." );
    }

    TEST_FIXTURE( NChooserEventCoordinatorFixture, Test_GH830 )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/Test_GH830.json",
                                       "The number of elements in 'Num_Targeted_Males' is 0.\nThe number of elements in 'Num_Targeted_Females' is 1.\nThe number of elements in 'Age_Range_Years' is 1.\n'Num_Targeted_Males', 'Num_Targeted_Females', and 'Age_Range_Years' must have the same number of elements, but not zero.  There must be one age range for each number targeted." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestWarningBaseYearStartYear)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/NChooserEventCoordinatorTest/TestSample.json"));
        FakeLogger fakeLogger(Logger::tLevel::WARNING);
        Environment::setLogger(&fakeLogger);
        
        Simulation::base_year = 2050;

        try
        {
            NChooserEventCoordinatorHIV hiv_nchooser;
            bool ret = hiv_nchooser.Configure(p_config.get());
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
