/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"

#include "MalariaDrugTypeParameters.h"
#include "GenomeMarkers.h"
#include "StrainIdentity.h"

using namespace std;
using namespace Kernel;

SUITE( MalariaDrugTypeParametersTest )
{
    TEST( TestInitialize )
    {
        std::vector<std::string> names;
        names.push_back( "Pfcrt" );
        names.push_back( "Pfdhps" );
        names.push_back( "Kelch13" );

        GenomeMarkers gm;
        gm.Initialize( names );

        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/MalariaDrugTypeParametersTest/TestInitialize.json" ) );

        unique_ptr<MalariaDrugTypeParameters> p_drug( MalariaDrugTypeParameters::CreateMalariaDrugTypeParameters( p_config.get(), "Chloroquine", gm ) );

        CHECK_EQUAL(  1.0f, p_drug->GetBodyWeightExponent() );
        CHECK_EQUAL(  2.0f, p_drug->GetCMax() );
        CHECK_EQUAL(  3.0f, p_drug->GetDecayT1() );
        CHECK_EQUAL(  4.0f, p_drug->GetDecayT2() );
        CHECK_EQUAL(  5.0f, p_drug->GetDoseInterval() );
        CHECK_EQUAL(  6,    p_drug->GetFullTreatmentDoses() );
        CHECK_EQUAL(  7.0f, p_drug->GetKillRateGametocyte02() );
        CHECK_EQUAL(  8.0f, p_drug->GetKillRateGametocyte34() );
        CHECK_EQUAL(  9.0f, p_drug->GetKillRateGametocyteM() );
        CHECK_EQUAL( 10.0f, p_drug->GetKillRateHepatocyte() );
        CHECK_EQUAL( 11.0f, p_drug->GetPkpdC50() );
        CHECK_EQUAL( 12.0f, p_drug->GetVd() );
        CHECK_EQUAL( 13.0f, p_drug->GetMaxDrugIRBCKill() );

        CHECK_EQUAL( std::string( "Kelch13" ), p_drug->GetResistantModifiers()[ 0 ].GetMarkerName() );
        CHECK_EQUAL( 0.5f, p_drug->GetResistantModifiers()[ 0 ].GetC50() );
        CHECK_EQUAL( 0.6f, p_drug->GetResistantModifiers()[ 0 ].GetMaxKilling() );

        CHECK_EQUAL( std::string( "Pfcrt"   ), p_drug->GetResistantModifiers()[ 1 ].GetMarkerName() );
        CHECK_EQUAL(  0.1f, p_drug->GetResistantModifiers()[ 1 ].GetC50() );
        CHECK_EQUAL(  0.2f, p_drug->GetResistantModifiers()[ 1 ].GetMaxKilling() );

        CHECK_EQUAL( std::string( "Pfdhps" ), p_drug->GetResistantModifiers()[ 2 ].GetMarkerName() );
        CHECK_EQUAL(  0.3f, p_drug->GetResistantModifiers()[ 2 ].GetC50() );
        CHECK_EQUAL(  0.4f, p_drug->GetResistantModifiers()[ 2 ].GetMaxKilling() );

        StrainIdentity strain;

        std::vector<std::string> markers;
        markers.push_back( "Pfcrt" );
        uint32_t genome = gm.CreateBits( markers );
        CHECK_EQUAL( 1, genome );
        strain.SetGeneticID( genome );
        CHECK_CLOSE( 0.1f, p_drug->GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.2f, p_drug->GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        markers.clear();

        markers.push_back( "Pfdhps" );
        genome = gm.CreateBits( markers );
        CHECK_EQUAL( 2, genome );
        strain.SetGeneticID( genome );
        CHECK_CLOSE( 0.3f, p_drug->GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.4f, p_drug->GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        markers.clear();

        markers.push_back( "Kelch13" );
        genome = gm.CreateBits( markers );
        CHECK_EQUAL( 4, genome );
        strain.SetGeneticID( genome );
        CHECK_CLOSE( 0.5f, p_drug->GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.6f, p_drug->GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        markers.clear();

        markers.push_back( "Pfcrt" );
        markers.push_back( "Pfdhps" );
        genome = gm.CreateBits( markers );
        CHECK_EQUAL( 3, genome );
        strain.SetGeneticID( genome );
        CHECK_CLOSE( 0.03f, p_drug->GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.08f, p_drug->GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        markers.clear();

        markers.push_back( "Pfcrt" );
        markers.push_back( "Kelch13" );
        genome = gm.CreateBits( markers );
        CHECK_EQUAL( 5, genome );
        strain.SetGeneticID( genome );
        CHECK_CLOSE( 0.05f, p_drug->GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.12f, p_drug->GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        markers.clear();

        markers.push_back( "Pfdhps" );
        markers.push_back( "Kelch13" );
        genome = gm.CreateBits( markers );
        CHECK_EQUAL( 6, genome );
        strain.SetGeneticID( genome );
        CHECK_CLOSE( 0.15f, p_drug->GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.24f, p_drug->GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        markers.clear();

        markers.push_back( "Pfcrt" );
        markers.push_back( "Pfdhps" );
        markers.push_back( "Kelch13" );
        genome = gm.CreateBits( markers );
        CHECK_EQUAL( 7, genome );
        strain.SetGeneticID( genome );
        CHECK_CLOSE( 0.015f, p_drug->GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.048f, p_drug->GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        markers.clear();

        markers.push_back( "Kelch13" );
        markers.push_back( "Pfdhps" );
        markers.push_back( "Pfcrt" );
        genome = gm.CreateBits( markers );
        CHECK_EQUAL( 7, genome );
        strain.SetGeneticID( genome );
        CHECK_CLOSE( 0.015f, p_drug->GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.048f, p_drug->GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );
    }

    void TestHelper_InitializeException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        try
        {
            std::vector<std::string> names;
            names.push_back( "Pfcrt" );
            names.push_back( "Pfdhps" );
            names.push_back( "Kelch13" );

            GenomeMarkers gm;
            gm.Initialize( names );

            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

            unique_ptr<MalariaDrugTypeParameters> p_drug( MalariaDrugTypeParameters::CreateMalariaDrugTypeParameters( p_config.get(), "Chloroquine", gm ) );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug( rExpMsg + std::string( "\n" ) );
                PrintDebug( msg     + std::string( "\n" ) );
                CHECK_LN( false, lineNumber );
            }
        }
    }

    TEST( TestBadJsonDoseMapAge )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadJsonDoseMapAge.json",
                                        "While trying to parse json data for param/key >>> Upper_Age_In_Years <<< in otherwise valid json segment.." );
    }

    TEST( TestBadJsonDoseMapFraction )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadJsonDoseMapFraction.json",
                                        "While trying to parse json data for param/key >>> Fraction_Of_Adult_Dose <<< in otherwise valid json segment.." );
    }

    TEST( TestOutOfRangeAgeLow )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestOutOfRangeAgeLow.json",
                                        "Variable Upper_Age_In_Years had value -16 which was inconsistent with range limit 0" );
    }

    TEST( TestOutOfRangeAgeHigh )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestOutOfRangeAgeHigh.json",
                                        "Variable Upper_Age_In_Years had value 999 which was inconsistent with range limit 125" );
    }

    TEST( TestOutOfRangeFractionLow )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestOutOfRangeFractionLow.json",
                                        "Variable Fraction_Of_Adult_Dose had value -1 which was inconsistent with range limit 0" );
    }

    TEST( TestOutOfRangeFractionHigh )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestOutOfRangeFractionHigh.json",
                                        "Variable Fraction_Of_Adult_Dose had value 77 which was inconsistent with range limit 1" );
    }

    TEST( TestMissingResistanceMarker )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestMissingResistanceMarker.json",
                                        "Cannot find GenomeMarkerModifiers for genome marker = Pfdhps" );
    }

    TEST( TestBadDoseInterval )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadDoseInterval.json",
                                        "time_between_doses (5) is less than dt (8)" );
    }

    TEST( TestMissingDrug )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestMissingDrug.json",
                                        "Object name not found: Chloroquine" );
    }
}
