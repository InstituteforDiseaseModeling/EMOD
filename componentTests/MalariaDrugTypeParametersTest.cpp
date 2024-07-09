
#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"

#include "MalariaDrugTypeParameters.h"
#include "StrainIdentityMalariaGenetics.h"
#include "ParasiteGenetics.h"
#include "ParasiteGenome.h"
#include "RANDOM.h"

using namespace std;
using namespace Kernel;

SUITE( MalariaDrugTypeParametersTest )
{
    struct DrugFixture
    {
        DrugFixture()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            JsonConfigurable::missing_parameters_set.clear();
            ParasiteGenome::ClearStatics();
        }

        ~DrugFixture()
        {
            ParasiteGenetics::CreateInstance()->ReduceGenomeMap();

            MalariaDrugTypeCollection::DeleteInstance();
            Environment::Finalize();

            ParasiteGenetics::DeleteInstance();
            ParasiteGenome::ClearStatics();
            JsonConfigurable::missing_parameters_set.clear();
        }
    };

    TEST_FIXTURE( DrugFixture, TestInitialize )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/MalariaDrugTypeParametersTest/TestInitialize.json" );

        unique_ptr<Configuration> pg_config( Configuration::CopyFromElement( (*EnvPtr->Config)[ "Parasite_Genetics" ], EnvPtr->Config->GetDataLocation() ) );
        ParasiteGenetics::CreateInstance()->Configure( pg_config.get() );

        MalariaDrugTypeCollection::GetInstanceNonConst()->ConfigureFromJsonAndKey( EnvPtr->Config, "Malaria_Drug_Params" );
        MalariaDrugTypeCollection::GetInstanceNonConst()->CheckConfiguration();

        const MalariaDrugTypeParameters& r_drug = MalariaDrugTypeCollection::GetInstance()->GetDrug( "Chloroquine" );

        CHECK_EQUAL(  1.0f, r_drug.GetBodyWeightExponent() );
        CHECK_EQUAL(  2.0f, r_drug.GetCMax() );
        CHECK_EQUAL(  3.0f, r_drug.GetDecayT1() );
        CHECK_EQUAL(  4.0f, r_drug.GetDecayT2() );
        CHECK_EQUAL(  5.0f, r_drug.GetDoseInterval() );
        CHECK_EQUAL(  6,    r_drug.GetFullTreatmentDoses() );
        CHECK_EQUAL(  7.0f, r_drug.GetKillRateGametocyte02() );
        CHECK_EQUAL(  8.0f, r_drug.GetKillRateGametocyte34() );
        CHECK_EQUAL(  9.0f, r_drug.GetKillRateGametocyteM() );
        CHECK_EQUAL( 10.0f, r_drug.GetKillRateHepatocyte() );
        CHECK_EQUAL( 11.0f, r_drug.GetPkpdC50() );
        CHECK_EQUAL( 12.0f, r_drug.GetVd() );
        CHECK_EQUAL( 13.0f, r_drug.GetMaxDrugIRBCKill() );

        // -----------------------------------------------------------------------------------
        // --- Test that the parasite is resistant when it has one of three different alleles
        // -----------------------------------------------------------------------------------
        CHECK_EQUAL( 3, r_drug.GetResistantModifiers().Size() );

        // Pfcrt
        CHECK_EQUAL( std::string( "T***" ), r_drug.GetResistantModifiers()[ 0 ]->GetDrugResistantString() );
        CHECK_EQUAL( 0.1f, r_drug.GetResistantModifiers()[ 0 ]->GetC50() );
        CHECK_EQUAL( 0.2f, r_drug.GetResistantModifiers()[ 0 ]->GetMaxKilling() );

        // Pfdhps
        CHECK_EQUAL( std::string( "*T**" ), r_drug.GetResistantModifiers()[ 1 ]->GetDrugResistantString() );
        CHECK_EQUAL(  0.3f, r_drug.GetResistantModifiers()[ 1 ]->GetC50() );
        CHECK_EQUAL(  0.4f, r_drug.GetResistantModifiers()[ 1 ]->GetMaxKilling() );

        // Kelch13
        CHECK_EQUAL( std::string( "**TT" ), r_drug.GetResistantModifiers()[ 2 ]->GetDrugResistantString() );
        CHECK_EQUAL(  0.5f, r_drug.GetResistantModifiers()[ 2 ]->GetC50() );
        CHECK_EQUAL(  0.6f, r_drug.GetResistantModifiers()[ 2 ]->GetMaxKilling() );

        // --------------------------------------------------------------------------
        // --- Create some genomes for testing that the right modifier is created
        // --------------------------------------------------------------------------
        PSEUDO_DES rng( 42 );

        std::string barcode = "AAA";
        std::string hrp     = "";

        std::string drug_aaaa = "AAAA";
        ParasiteGenome pg_tmp_aaaa = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode, drug_aaaa, hrp );
        ParasiteGenome pg_aaaa = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_aaaa, 1 );

        std::string drug_taaa = "TAAA"; // Pfcrt
        ParasiteGenome pg_tmp_taaa = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode, drug_taaa, hrp );
        ParasiteGenome pg_taaa = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_taaa, 1 );

        std::string drug_ataa = "ATAA"; // Pfdhps
        ParasiteGenome pg_tmp_ataa = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode, drug_ataa, hrp );
        ParasiteGenome pg_ataa = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_ataa, 1 );

        std::string drug_aata = "AATA";
        ParasiteGenome pg_tmp_aata = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode, drug_aata, hrp );
        ParasiteGenome pg_aata = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_aata, 1 );

        std::string drug_aatt = "AATT"; // Kelch13
        ParasiteGenome pg_tmp_aatt = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode, drug_aatt, hrp );
        ParasiteGenome pg_aatt = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_aatt, 1 );

        std::string drug_cctt = "CCTT"; // Kelch13
        ParasiteGenome pg_tmp_cctt = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode, drug_cctt, hrp );
        ParasiteGenome pg_cctt = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_cctt, 1 );

        std::string drug_tatt = "TATT"; // Pfcrt & Kelch13
        ParasiteGenome pg_tmp_tatt = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode, drug_tatt, hrp );
        ParasiteGenome pg_tatt = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_tatt, 1 );

        std::string drug_tttt = "TTTT"; // Pfcrt & Pfdhps & Kelch13
        ParasiteGenome pg_tmp_tttt = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode, drug_tttt, hrp );
        ParasiteGenome pg_tttt = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_tttt, 1 );

        CHECK_EQUAL( barcode, pg_aaaa.GetBarcode() );
        CHECK_EQUAL( barcode, pg_taaa.GetBarcode() );
        CHECK_EQUAL( barcode, pg_ataa.GetBarcode() );
        CHECK_EQUAL( barcode, pg_aata.GetBarcode() );
        CHECK_EQUAL( barcode, pg_aatt.GetBarcode() );
        CHECK_EQUAL( barcode, pg_cctt.GetBarcode() );
        CHECK_EQUAL( barcode, pg_tatt.GetBarcode() );
        CHECK_EQUAL( barcode, pg_tttt.GetBarcode() );

        CHECK_EQUAL( drug_aaaa, pg_aaaa.GetDrugResistantString() );
        CHECK_EQUAL( drug_taaa, pg_taaa.GetDrugResistantString() );
        CHECK_EQUAL( drug_ataa, pg_ataa.GetDrugResistantString() );
        CHECK_EQUAL( drug_aata, pg_aata.GetDrugResistantString() );
        CHECK_EQUAL( drug_aatt, pg_aatt.GetDrugResistantString() );
        CHECK_EQUAL( drug_cctt, pg_cctt.GetDrugResistantString() );
        CHECK_EQUAL( drug_tatt, pg_tatt.GetDrugResistantString() );
        CHECK_EQUAL( drug_tttt, pg_tttt.GetDrugResistantString() );

        // ------------------------------------------------------------------------------
        // --- Test that the correct modification is created depending on the particular
        // --- set of resistant markers that are set.
        // ------------------------------------------------------------------------------
        StrainIdentityMalariaGenetics strain( pg_aaaa );

        CHECK_CLOSE( 1.0f, r_drug.GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 1.0f, r_drug.GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        strain.SetGenome( pg_taaa );

        CHECK_CLOSE( 0.1f, r_drug.GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.2f, r_drug.GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        strain.SetGenome( pg_ataa );

        CHECK_CLOSE( 0.3f, r_drug.GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.4f, r_drug.GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        strain.SetGenome( pg_aata );

        CHECK_CLOSE( 1.0f, r_drug.GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 1.0f, r_drug.GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        strain.SetGenome( pg_aatt );

        CHECK_CLOSE( 0.5f, r_drug.GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.6f, r_drug.GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        strain.SetGenome( pg_cctt );

        CHECK_CLOSE( 0.5f, r_drug.GetResistantModifiers().GetC50(        strain ), 0.00001 );
        CHECK_CLOSE( 0.6f, r_drug.GetResistantModifiers().GetMaxKilling( strain ), 0.00001 );

        strain.SetGenome( pg_tatt );

        CHECK_CLOSE( 0.05f, r_drug.GetResistantModifiers().GetC50(        strain ), 0.00001 ); // 0.1 * 0.5
        CHECK_CLOSE( 0.12f, r_drug.GetResistantModifiers().GetMaxKilling( strain ), 0.00001 ); // 0.2 * 0.6

        strain.SetGenome( pg_tttt );

        CHECK_CLOSE( 0.015f, r_drug.GetResistantModifiers().GetC50(        strain ), 0.00001 ); // 0.1 * 0.3 * 0.5
        CHECK_CLOSE( 0.048f, r_drug.GetResistantModifiers().GetMaxKilling( strain ), 0.00001 ); // 0.2 * 0.4 * 0.6
    }

    void TestHelper_InitializeException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        JsonConfigurable::_useDefaults = false;
        JsonConfigurable::_track_missing = false;
        try
        {
            EnvPtr->Config = Environment::LoadConfigurationFile( rFilename );

            unique_ptr<Configuration> pg_config( Configuration::CopyFromElement( (*EnvPtr->Config)[ "Parasite_Genetics" ], EnvPtr->Config->GetDataLocation() ) );
            ParasiteGenetics::CreateInstance()->Configure( pg_config.get() );

            MalariaDrugTypeCollection::GetInstanceNonConst()->ConfigureFromJsonAndKey( EnvPtr->Config, "Malaria_Drug_Params" );
            MalariaDrugTypeCollection::GetInstanceNonConst()->CheckConfiguration();

            const MalariaDrugTypeParameters& r_drug = MalariaDrugTypeCollection::GetInstance()->GetDrug( "Chloroquine" );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug(           std::string( "\n" ) );
                PrintDebug( rExpMsg + std::string( "\n" ) );
                PrintDebug( msg     + std::string( "\n" ) );
                CHECK_LN( false, lineNumber );
            }
        }
    }

    TEST_FIXTURE( DrugFixture, TestBadJsonDoseMapAge )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadJsonDoseMapAge.json",
                                        "Parameter 'Upper_Age_In_Years of DoseFractionByAge' not found in input file 'testdata/MalariaDrugTypeParametersTest/TestBadJsonDoseMapAge.json'." );
    }

    TEST_FIXTURE( DrugFixture, TestBadJsonDoseMapFraction )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadJsonDoseMapFraction.json",
                                        "Parameter 'Fraction_Of_Adult_Dose of DoseFractionByAge' not found in input file 'testdata/MalariaDrugTypeParametersTest/TestBadJsonDoseMapFraction.json'." );
    }

    TEST_FIXTURE( DrugFixture, TestOutOfRangeAgeLow )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestOutOfRangeAgeLow.json",
                                        "Configuration variable 'Upper_Age_In_Years' with value -16 out of range: less than 0." );
    }

    TEST_FIXTURE( DrugFixture, TestOutOfRangeAgeHigh )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestOutOfRangeAgeHigh.json",
                                        "Configuration variable 'Upper_Age_In_Years' with value 999 out of range: greater than 125." );
    }

    TEST_FIXTURE( DrugFixture, TestOutOfRangeFractionLow )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestOutOfRangeFractionLow.json",
                                        "Configuration variable 'Fraction_Of_Adult_Dose' with value -1 out of range: less than 0." );
    }

    TEST_FIXTURE( DrugFixture, TestOutOfRangeFractionHigh )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestOutOfRangeFractionHigh.json",
                                        "Configuration variable 'Fraction_Of_Adult_Dose' with value 77 out of range: greater than 1." );
    }

    TEST_FIXTURE( DrugFixture, TestBadDrugResistantStringA )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadDrugResistantStringA.json",
                                        "The 'Drug_Resistant_String' = 'T' is invalid.\nIt has 1 characters and 'Drug_Resistant_Genome_Locations' says you must have 4." );
    }

    TEST_FIXTURE( DrugFixture, TestBadDrugResistantStringB )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadDrugResistantStringB.json",
                                        "The character 'Z' in the parameter 'Drug_Resistant_String' is invalid.\nValid values are: 'A', 'C', 'G', 'T'" );
    }

    TEST_FIXTURE( DrugFixture, TestBadDrugResistantStringC )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadDrugResistantStringC.json",
                                        "The 'Drug_Resistant_String' = 'AAATTT' is invalid.\nIt has 6 characters and 'Drug_Resistant_Genome_Locations' says you must have 4." );
    }

    TEST_FIXTURE( DrugFixture, TestBadDrugResistantStringD )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadDrugResistantStringD.json",
                                        "Invalid parameter 'Drug_Resistant_String' = '****'\nThe string must define at least one value.  It cannot be all wild cards ('*').\nValid values are: 'A', 'C', 'G', 'T'" );
    }

    TEST_FIXTURE( DrugFixture, TestBadDrugResistantStringE )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadDrugResistantStringE.json",
                                        "Invalid parameter 'Drug_Resistant_String' = '' and 'Drug_Resistant_Genome_Locations' with zero locations.\nYou must define some drug resistant locations in the genome before you can use drug resistance." );
    }

    TEST_FIXTURE( DrugFixture, TestBadDoseInterval )
    {
        std::string exp_msg;
        exp_msg += "Invalid 'Drug_Dose_Interval' in drug 'Chloroquine'.\n";
        exp_msg += "'Drug_Dose_Interval'(=5) is less than the drug update period.\n";
        exp_msg += "The drug update period = 'Simulation_Timestep'(=8) / 'Infection_Updates_Per_Timestep'(=1) = 8.\n";
        exp_msg += "Please change these parameters so that 'Drug_Dose_Interval' >= the drug update period.";

        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestBadDoseInterval.json", exp_msg );
    }

    TEST_FIXTURE( DrugFixture, TestMissingDrug )
    {
        TestHelper_InitializeException( __LINE__, "testdata/MalariaDrugTypeParametersTest/TestMissingDrug.json",
                                        "'Chloroquine' is an unknown drug.\nValid drug names are:\nBAD" );
    }
}
