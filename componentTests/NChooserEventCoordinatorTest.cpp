/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "NChooserEventCoordinatorHIV.h"
#include "Node.h"
#include "SimulationConfig.h"

#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "IdmMpi.h"


using namespace std; 
using namespace Kernel; 

extern void PrintDebug( const std::string& rMessage );


SUITE(NChooserEventCoordinatorTest)
{
    static int m_NextId = 1 ;

    struct NChooserEventCoordinatorFixture
    {
        IdmMpi::MessageInterface* m_pMpi;
        SimulationConfig* m_pSimulationConfig ;

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

            Environment::Initialize( m_pMpi, nullptr, configFilename, inputPath, outputPath, /*statePath, */dllPath, false);

            const_cast<Environment*>(Environment::getInstance())->RNG = new PSEUDO_DES(0);

            m_pSimulationConfig->listed_events.insert( "Vaccinated" );
            m_pSimulationConfig->listed_events.insert( "VaccineExpired" );

            Environment::setSimulationConfig( m_pSimulationConfig );
            Node::TestOnly_ClearProperties();
        }

        ~NChooserEventCoordinatorFixture()
        {
            //for( auto hic : m_hic_list )
            //{
            //    delete hic ;
            //}
            //m_hic_list.clear();

            //for( auto human : m_human_list )
            //{
            //    delete human ;
            //}
            //m_human_list.clear();
            Node::TestOnly_ClearProperties();
            Environment::Finalize();
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
        p_human->GetProperties()->operator[]( rPropertyName ) = rPropertyValue ;

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
        Node::TestOnly_AddPropertyKeyValue( "Location", "URBAN" );
        Node::TestOnly_AddPropertyKeyValue( "Location", "RURAL" );

        INodeContextFake nc;
        INodeEventContextFake nec;

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

        DiseaseQualifications disease_qual;
        PropertyRestrictions pr;

        TargetedByAgeAndGender ag1( AgeRange( 15.0, 30.0 ), Gender::COUNT, 10, 3, 0 );

        ag1.FindQualifyingIndividuals( &nec, disease_qual, pr );

        std::vector<IIndividualHumanEventContext*> selected_list_1 = ag1.SelectIndividuals();
        CHECK_EQUAL(  4, selected_list_1.size() );
        CHECK_EQUAL(  4, selected_list_1[0]->GetSuid().data );
        CHECK_EQUAL(  8, selected_list_1[1]->GetSuid().data );
        CHECK_EQUAL( 12, selected_list_1[2]->GetSuid().data );
        CHECK_EQUAL( 14, selected_list_1[3]->GetSuid().data );

        ag1.IncrementNextNumTargets();

        ag1.FindQualifyingIndividuals( &nec, disease_qual, pr );

        std::vector<IIndividualHumanEventContext*> selected_list_2 = ag1.SelectIndividuals();
        CHECK_EQUAL(  3, selected_list_2.size() );
        CHECK_EQUAL( 12, selected_list_2[0]->GetSuid().data );
        CHECK_EQUAL( 16, selected_list_2[1]->GetSuid().data );
        CHECK_EQUAL( 19, selected_list_2[2]->GetSuid().data );

        ag1.IncrementNextNumTargets();

        ag1.FindQualifyingIndividuals( &nec, disease_qual, pr );

        std::vector<IIndividualHumanEventContext*> selected_list_3 = ag1.SelectIndividuals();
        CHECK_EQUAL(  3, selected_list_3.size() );
        CHECK_EQUAL( 11, selected_list_3[0]->GetSuid().data );
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

    void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        try
        {
            Node::TestOnly_AddPropertyKeyValue( "Location", "URBAN" );
            Node::TestOnly_AddPropertyKeyValue( "Location", "RURAL" );
            Node::TestOnly_AddPropertyKeyValue( "Income",   "LOW"   );
            Node::TestOnly_AddPropertyKeyValue( "Income",   "MED"   );
            Node::TestOnly_AddPropertyKeyValue( "Income",   "HIGH"  );

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
            "Variable or parameter 'Num_Targeted_Males' with value 0 is incompatible with variable or parameter 'Age_Ranges_Years' with value 0. Num_Targeted_Males, Num_Targeted_Females, and Age_Range_Years must have the same number of elements, but not zero.  There should be one age range for each number targeted." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidNumTargeted)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidNumTargeted.json",
            "Variable or parameter 'Num_Targeted' with value 4 is incompatible with variable or parameter 'Age_Ranges_Years' with value 1. Num_Targeted and Age_Range_Years must have the same number of elements, but not zero.  There should be one age range for each number targeted." );
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
            "Variable or parameter 'Num_Targeted_Males' with value 1 is incompatible with variable or parameter 'Age_Ranges_Years' with value 2. Num_Targeted_Males, Num_Targeted_Females, and Age_Range_Years must have the same number of elements, but not zero.  There should be one age range for each number targeted." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidNumTargetedFemales)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidNumTargetedFemales.json",
            "Variable or parameter 'Num_Targeted_Males' with value 2 is incompatible with variable or parameter 'Age_Ranges_Years' with value 2. Num_Targeted_Males, Num_Targeted_Females, and Age_Range_Years must have the same number of elements, but not zero.  There should be one age range for each number targeted." );
    }

    TEST_FIXTURE(NChooserEventCoordinatorFixture, TestInvalidNumAgeRanges)
    {
        TestHelper_ConfigureException( __LINE__, "testdata/NChooserEventCoordinatorTest/TestInvalidNumAgeRanges.json",
            "Variable or parameter 'Num_Targeted_Males' with value 2 is incompatible with variable or parameter 'Age_Ranges_Years' with value 1. Num_Targeted_Males, Num_Targeted_Females, and Age_Range_Years must have the same number of elements, but not zero.  There should be one age range for each number targeted." );
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
}
