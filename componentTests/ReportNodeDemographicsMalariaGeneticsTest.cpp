
#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"
#include "ParasiteGenetics.h"
#include "ParasiteGenome.h"
#include "ReportNodeDemographicsMalariaGenetics.h"
#include "RANDOM.h"
#include "Instrumentation.h"

using namespace Kernel;


SUITE( ReportNodeDemographicsMalariaGeneticsTest )
{
    struct GeneticsFixture
    {
        GeneticsFixture()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            JsonConfigurable::missing_parameters_set.clear();
            ParasiteGenome::ClearStatics();

            //SusceptibilityMalariaConfig::falciparumMSPVars      = DEFAULT_MSP_VARIANTS;
            //SusceptibilityMalariaConfig::falciparumNonSpecTypes = DEFAULT_NONSPECIFIC_TYPES;
            //SusceptibilityMalariaConfig::falciparumPfEMP1Vars   = DEFAULT_PFEMP1_VARIANTS;
        }

        ~GeneticsFixture()
        {
            ParasiteGenetics::CreateInstance()->ReduceGenomeMap();

            Environment::Finalize();
            ParasiteGenetics::DeleteInstance();
            ParasiteGenome::ClearStatics();
            JsonConfigurable::missing_parameters_set.clear();
        }
    };

#if 1
    TEST_FIXTURE( GeneticsFixture, TestCalculateFractionInfo )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ReportNodeDemographicsMalariaGeneticsTest/TestCalculateFractionInfo.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        std::string row_barcode     = "AAAAAAAAAAAAAAAAAAAAAAAA";
        std::string col_barcode_0   = "CCCCCCCCCCCCCCCCCCCCCCCC";
        std::string col_barcode_50a = "CCCCCCCCCCCCAAAAAAAAAAAA";
        std::string col_barcode_50b = "AAAAAAAAAAAACCCCCCCCCCCC";
        std::string col_barcode_50c = "ACACACACACACACACACACACAC";
        std::string col_barcode_75a = "CCCCCCAAAAAAAAAAAAAAAAAA";
        std::string col_barcode_75b = "AAAAAACCCCCCAAAAAAAAAAAA";
        std::string col_barcode_75c = "AAAAAAAAAAAACCCCCCAAAAAA";
        std::string col_barcode_75d = "AAAAAAAAAAAAAAAAAACCCCCC";
        std::string col_barcode_75e = "CAAACAAACAAACAAACAAACAAA";
        std::string col_barcode_75f = "ACAAACAAACAAACAAACAAACAA";
        std::string col_barcode_75g = "AACAAACAAACAAACAAACAAACA";
        std::string col_barcode_75h = "AAACAAACAAACAAACAAACAAAC";
        std::string col_barcode_1   = "CCCCCCCCCCCCACCCCCCCCCCC";

        std::vector<int> row_roots     = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> col_roots_0   = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        std::vector<int> col_roots_50a = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> col_roots_50b = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        std::vector<int> col_roots_50c = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
        std::vector<int> col_roots_75a = { 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> col_roots_75b = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> col_roots_75c = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };
        std::vector<int> col_roots_75d = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 };
        std::vector<int> col_roots_75e = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
        std::vector<int> col_roots_75f = { 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0 };
        std::vector<int> col_roots_75g = { 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0 };
        std::vector<int> col_roots_75h = { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1 };
        std::vector<int> col_roots_1   = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

        ParasiteGenome row_genome     = ParasiteGenetics::GetInstance()->CreateGenome( row_barcode,     row_roots     );
        ParasiteGenome col_genome_0   = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_0,   col_roots_0   );
        ParasiteGenome col_genome_50a = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_50a, col_roots_50a );
        ParasiteGenome col_genome_50b = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_50b, col_roots_50b );
        ParasiteGenome col_genome_50c = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_50c, col_roots_50c );
        ParasiteGenome col_genome_75a = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_75a, col_roots_75a );
        ParasiteGenome col_genome_75b = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_75b, col_roots_75b );
        ParasiteGenome col_genome_75c = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_75c, col_roots_75c );
        ParasiteGenome col_genome_75d = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_75d, col_roots_75d );
        ParasiteGenome col_genome_75e = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_75e, col_roots_75e );
        ParasiteGenome col_genome_75f = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_75f, col_roots_75f );
        ParasiteGenome col_genome_75g = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_75g, col_roots_75g );
        ParasiteGenome col_genome_75h = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_75h, col_roots_75h );
        ParasiteGenome col_genome_1   = ParasiteGenetics::GetInstance()->CreateGenome( col_barcode_1,   col_roots_1   );

        // ------------------------------
        // --- Test Completely Different
        // ------------------------------
        GenomeComboMatrixColumn col_0( row_genome, col_genome_0 );

        CHECK_EQUAL( 0.0, col_0.GetFractionSameAllele() );
        CHECK_EQUAL( 0.0, col_0.GetFractionSameRoot()   );

        // ------------------------------
        // --- Test Completely Same
        // ------------------------------
        GenomeComboMatrixColumn col_0_0( col_genome_0, col_genome_0 );

        CHECK_EQUAL( 1.0, col_0_0.GetFractionSameAllele() );
        CHECK_EQUAL( 1.0, col_0_0.GetFractionSameRoot()   );

        // ------------------------------
        // --- Test 50% Same
        // ------------------------------
        GenomeComboMatrixColumn col_50a( row_genome, col_genome_50a );

        CHECK_EQUAL( 0.5, col_50a.GetFractionSameAllele() );
        CHECK_EQUAL( 0.5, col_50a.GetFractionSameRoot()   );

        GenomeComboMatrixColumn col_50b( row_genome, col_genome_50b );

        CHECK_EQUAL( 0.5, col_50b.GetFractionSameAllele() );
        CHECK_EQUAL( 0.5, col_50b.GetFractionSameRoot()   );

        GenomeComboMatrixColumn col_50c( row_genome, col_genome_50c );

        CHECK_EQUAL( 0.5, col_50c.GetFractionSameAllele() );
        CHECK_EQUAL( 0.5, col_50c.GetFractionSameRoot()   );

        GenomeComboMatrixColumn col_50a_50c( col_genome_50a, col_genome_50c );

        CHECK_EQUAL( 0.5, col_50a_50c.GetFractionSameAllele() );
        CHECK_EQUAL( 0.5, col_50a_50c.GetFractionSameRoot()   );

        // ------------------------------------
        // --- Test Different 50% is Different
        // ------------------------------------
        GenomeComboMatrixColumn col_50a_50b( col_genome_50a, col_genome_50b );

        CHECK_EQUAL( 0.0, col_50a_50b.GetFractionSameAllele() );
        CHECK_EQUAL( 0.0, col_50a_50b.GetFractionSameRoot()   );

        // ------------------------------
        // --- Test 75% Same
        // ------------------------------
        GenomeComboMatrixColumn col_75a( row_genome, col_genome_75a );

        CHECK_EQUAL( 0.75, col_75a.GetFractionSameAllele() );
        CHECK_EQUAL( 0.75, col_75a.GetFractionSameRoot()   );

        GenomeComboMatrixColumn col_75b( row_genome, col_genome_75b );

        CHECK_EQUAL( 0.75, col_75b.GetFractionSameAllele() );
        CHECK_EQUAL( 0.75, col_75b.GetFractionSameRoot()   );

        GenomeComboMatrixColumn col_75c( row_genome, col_genome_75c );

        CHECK_EQUAL( 0.75, col_75c.GetFractionSameAllele() );
        CHECK_EQUAL( 0.75, col_75c.GetFractionSameRoot()   );

        GenomeComboMatrixColumn col_75d( row_genome, col_genome_75d );

        CHECK_EQUAL( 0.75, col_75d.GetFractionSameAllele() );
        CHECK_EQUAL( 0.75, col_75d.GetFractionSameRoot()   );

        GenomeComboMatrixColumn col_75e( row_genome, col_genome_75e );

        CHECK_EQUAL( 0.75, col_75e.GetFractionSameAllele() );
        CHECK_EQUAL( 0.75, col_75e.GetFractionSameRoot()   );

        GenomeComboMatrixColumn col_75f( row_genome, col_genome_75f );

        CHECK_EQUAL( 0.75, col_75f.GetFractionSameAllele() );
        CHECK_EQUAL( 0.75, col_75f.GetFractionSameRoot()   );

        GenomeComboMatrixColumn col_75g( row_genome, col_genome_75g );

        CHECK_EQUAL( 0.75, col_75g.GetFractionSameAllele() );
        CHECK_EQUAL( 0.75, col_75g.GetFractionSameRoot()   );

        GenomeComboMatrixColumn col_75h( row_genome, col_genome_75h );

        CHECK_EQUAL( 0.75, col_75h.GetFractionSameAllele() );
        CHECK_EQUAL( 0.75, col_75h.GetFractionSameRoot()   );

        // ------------------------------
        // --- Test 25% Same
        // ------------------------------
        GenomeComboMatrixColumn col_0_75a( col_genome_0, col_genome_75a );

        CHECK_EQUAL( 0.25, col_0_75a.GetFractionSameAllele() );
        CHECK_EQUAL( 0.25, col_0_75a.GetFractionSameRoot()   );

        // ------------------------------
        // --- Test 1 Same
        // ------------------------------
        GenomeComboMatrixColumn col_1( row_genome, col_genome_1 );

        CHECK_CLOSE( (1.0/24.0), col_1.GetFractionSameAllele(), FLT_EPSILON );
        CHECK_CLOSE( (1.0/24.0), col_1.GetFractionSameRoot()  , FLT_EPSILON );

        // ------------------------------
        // --- Test 23 Same
        // ------------------------------
        GenomeComboMatrixColumn col_0_1( col_genome_0, col_genome_1 );

        CHECK_CLOSE( (23.0/24.0), col_0_1.GetFractionSameAllele(), FLT_EPSILON );
        CHECK_CLOSE( (23.0/24.0), col_0_1.GetFractionSameRoot()  , FLT_EPSILON );
    }

    TEST_FIXTURE( GeneticsFixture, TestCalculateFractionInfoForNumPositionsNotDivisibleBy8 )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ReportNodeDemographicsMalariaGeneticsTest/TestCalculateFractionInfoForNumPositionsNotDivisibleBy8.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        std::string barcode_0   = "AAAAAAAAAAAAAAAAAAAAAAAAAA";
        std::string barcode_100 = "CCCCCCCCCCCCCCCCCCCCCCCCCC";
        std::string barcode_50a = "CCCCCCCCCCCCCAAAAAAAAAAAAA";
        std::string barcode_25e = "CAAACAAACAAACAAACAAACAAACA";

        std::vector<int> roots_0   = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> roots_100 = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        std::vector<int> roots_50a = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> roots_25e = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0 };

        ParasiteGenome genome_0   = ParasiteGenetics::GetInstance()->CreateGenome( barcode_0,   roots_0   );
        ParasiteGenome genome_100 = ParasiteGenetics::GetInstance()->CreateGenome( barcode_100, roots_100 );
        ParasiteGenome genome_50a = ParasiteGenetics::GetInstance()->CreateGenome( barcode_50a, roots_50a );
        ParasiteGenome genome_25e = ParasiteGenetics::GetInstance()->CreateGenome( barcode_25e, roots_25e );

        // ------------------------------
        // --- Test Completely Different
        // ------------------------------
        GenomeComboMatrixColumn col_0_100( genome_0, genome_100 );

        CHECK_EQUAL( 0.0, col_0_100.GetFractionSameAllele() );
        CHECK_EQUAL( 0.0, col_0_100.GetFractionSameRoot()   );

        // ------------------------------
        // --- Test 50% Same
        // ------------------------------
        GenomeComboMatrixColumn col_0_50a( genome_0, genome_50a );

        CHECK_EQUAL( 0.5, col_0_50a.GetFractionSameAllele() );
        CHECK_EQUAL( 0.5, col_0_50a.GetFractionSameRoot()   );

        // ------------------------------
        // --- Test 19/26 the same
        // ------------------------------
        GenomeComboMatrixColumn col_0_25e( genome_0, genome_25e );

        CHECK_CLOSE( 0.7307692, col_0_25e.GetFractionSameAllele(), FLT_EPSILON );
        CHECK_CLOSE( 0.7307692, col_0_25e.GetFractionSameRoot()  , FLT_EPSILON );
    }

    TEST_FIXTURE( GeneticsFixture, TestCalculateNumCombinations )
    {
        GenomeComboMatrix matrix( 365.0 );

        CHECK_EQUAL(  0, matrix.CalculateNumCombinations(  0 ) );
        CHECK_EQUAL(  0, matrix.CalculateNumCombinations(  1 ) );
        CHECK_EQUAL(  1, matrix.CalculateNumCombinations(  2 ) );
        CHECK_EQUAL(  3, matrix.CalculateNumCombinations(  3 ) );
        CHECK_EQUAL(  6, matrix.CalculateNumCombinations(  4 ) );
        CHECK_EQUAL( 10, matrix.CalculateNumCombinations(  5 ) );
        CHECK_EQUAL( 15, matrix.CalculateNumCombinations(  6 ) );
        CHECK_EQUAL( 21, matrix.CalculateNumCombinations(  7 ) );
        CHECK_EQUAL( 28, matrix.CalculateNumCombinations(  8 ) );
        CHECK_EQUAL( 36, matrix.CalculateNumCombinations(  9 ) );
        CHECK_EQUAL( 45, matrix.CalculateNumCombinations( 10 ) );
    }

    TEST_FIXTURE( GeneticsFixture, TestGenomeComboMatrix )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ReportNodeDemographicsMalariaGeneticsTest/TestGenomeComboMatrix.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        std::string barcode_0   = "AAAAAAAAAAAAAAAAAAAAAAAA";
        std::string barcode_100 = "CCCCCCCCCCCCCCCCCCCCCCCC";
        std::string barcode_50a = "CCCCCCCCCCCCAAAAAAAAAAAA";
        std::string barcode_50b = "AAAAAAAAAAAACCCCCCCCCCCC";
        std::string barcode_50c = "ACACACACACACACACACACACAC";
        std::string barcode_25a = "CCCCCCAAAAAAAAAAAAAAAAAA";
        std::string barcode_25b = "AAAAAACCCCCCAAAAAAAAAAAA";
        std::string barcode_25c = "AAAAAAAAAAAACCCCCCAAAAAA";
        std::string barcode_25d = "AAAAAAAAAAAAAAAAAACCCCCC";
        std::string barcode_25e = "CAAACAAACAAACAAACAAACAAA";
        std::string barcode_25f = "ACAAACAAACAAACAAACAAACAA";
        std::string barcode_25g = "AACAAACAAACAAACAAACAAACA";
        std::string barcode_25h = "AAACAAACAAACAAACAAACAAAC";

        std::vector<int> roots_0   = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> roots_100 = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        std::vector<int> roots_50a = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> roots_50b = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        std::vector<int> roots_50c = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
        std::vector<int> roots_25a = { 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> roots_25b = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> roots_25c = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };
        std::vector<int> roots_25d = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 };
        std::vector<int> roots_25e = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
        std::vector<int> roots_25f = { 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0 };
        std::vector<int> roots_25g = { 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0 };
        std::vector<int> roots_25h = { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1 };

        ParasiteGenome genome_0   = ParasiteGenetics::GetInstance()->CreateGenome( barcode_0,   roots_0   );
        ParasiteGenome genome_100 = ParasiteGenetics::GetInstance()->CreateGenome( barcode_100, roots_100 );
        ParasiteGenome genome_50a = ParasiteGenetics::GetInstance()->CreateGenome( barcode_50a, roots_50a );
        ParasiteGenome genome_50b = ParasiteGenetics::GetInstance()->CreateGenome( barcode_50b, roots_50b );
        ParasiteGenome genome_50c = ParasiteGenetics::GetInstance()->CreateGenome( barcode_50c, roots_50c );
        ParasiteGenome genome_25a = ParasiteGenetics::GetInstance()->CreateGenome( barcode_25a, roots_25a );
        ParasiteGenome genome_25b = ParasiteGenetics::GetInstance()->CreateGenome( barcode_25b, roots_25b );
        ParasiteGenome genome_25c = ParasiteGenetics::GetInstance()->CreateGenome( barcode_25c, roots_25c );
        ParasiteGenome genome_25d = ParasiteGenetics::GetInstance()->CreateGenome( barcode_25d, roots_25d );
        ParasiteGenome genome_25e = ParasiteGenetics::GetInstance()->CreateGenome( barcode_25e, roots_25e );
        ParasiteGenome genome_25f = ParasiteGenetics::GetInstance()->CreateGenome( barcode_25f, roots_25f );
        ParasiteGenome genome_25g = ParasiteGenetics::GetInstance()->CreateGenome( barcode_25g, roots_25g );
        ParasiteGenome genome_25h = ParasiteGenetics::GetInstance()->CreateGenome( barcode_25h, roots_25h );

        // ------------------------------
        // --- Test Empty Matrix
        // ------------------------------
        float avg_ibs = 0.0;
        float avg_ibd = 0.0;

        GenomeComboMatrix matrix( 365.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(     0, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     0, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_EQUAL( 0.0, avg_ibs );
        CHECK_EQUAL( 0.0, avg_ibd );

        // ------------------------------
        // --- Test Matrix with one Genome
        // ------------------------------

        matrix.AddGenome( genome_0, 0.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(     0, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     1, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_EQUAL( 0.0, avg_ibs );
        CHECK_EQUAL( 0.0, avg_ibd );

        // ---------------------------------
        // --- Test Matrix with two genomes
        // ---------------------------------

        matrix.AddGenome( genome_100, 10.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(     1, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     2, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_EQUAL( 0.0, avg_ibs );
        CHECK_EQUAL( 0.0, avg_ibd );

        // ---------------------------------
        // --- Test Matrix with 3 genomes
        // ---------------------------------

        matrix.AddGenome( genome_50a, 20.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(     3, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     3, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_CLOSE( 0.3333333, avg_ibs, FLT_EPSILON );
        CHECK_CLOSE( 0.3333333, avg_ibd, FLT_EPSILON );

        // ---------------------------------
        // --- Test Matrix with 4 genomes
        // ---------------------------------

        matrix.AddGenome( genome_25a, 30.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(     6, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     4, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_CLOSE( 0.4583333, avg_ibs, FLT_EPSILON );
        CHECK_CLOSE( 0.4583333, avg_ibd, FLT_EPSILON );

        // ---------------------------------
        // --- Test Matrix with 5 genomes
        // ---------------------------------

        matrix.AddGenome( genome_25e, 40.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(    10, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     5, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_CLOSE( 0.4916667, avg_ibs, FLT_EPSILON );
        CHECK_CLOSE( 0.4916667, avg_ibd, FLT_EPSILON );

        // ---------------------------------
        // --- Test Matrix with 6 genomes
        // ---------------------------------

        matrix.AddGenome( genome_25h, 50.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(    15, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     6, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_CLOSE( 0.5, avg_ibs, FLT_EPSILON );
        CHECK_CLOSE( 0.5, avg_ibd, FLT_EPSILON );

        // ---------------------------------
        // --- Test Matrix - Remove Genome - 6 -> 5
        // ---------------------------------

        matrix.RemoveOld( 366.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(    10, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     5, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_CLOSE( 0.475, avg_ibs, FLT_EPSILON );
        CHECK_CLOSE( 0.475, avg_ibd, FLT_EPSILON );

        // ---------------------------------
        // --- Test Matrix - Remove Genome - 5 -> 4
        // ---------------------------------

        matrix.RemoveOld( 376.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(     6, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     4, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_CLOSE( 0.5833333, avg_ibs, FLT_EPSILON );
        CHECK_CLOSE( 0.5833333, avg_ibd, FLT_EPSILON );

        // ---------------------------------
        // --- Test Matrix - Add Genome - 4 -> 5
        // ---------------------------------

        matrix.AddGenome( genome_50c, 376.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(    10, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     5, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_CLOSE( 0.55, avg_ibs, FLT_EPSILON );
        CHECK_CLOSE( 0.55, avg_ibd, FLT_EPSILON );

        // ---------------------------------
        // --- Test Matrix - Add Genome - 5 -> 6
        // ---------------------------------

        matrix.AddGenome( genome_25b, 377.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(    15, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     6, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_CLOSE( 0.5666667, avg_ibs, FLT_EPSILON );
        CHECK_CLOSE( 0.5666667, avg_ibd, FLT_EPSILON );

        // ---------------------------------
        // --- Test Matrix - Remove Genome - 6 -> 4
        // ---------------------------------

        matrix.RemoveOld( 396.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(     6, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     4, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_CLOSE( 0.5416667, avg_ibs, FLT_EPSILON );
        CHECK_CLOSE( 0.5416667, avg_ibd, FLT_EPSILON );

        // ---------------------------------
        // --- Test Matrix - Remove Genome ALL
        // ---------------------------------

        matrix.RemoveOld( 1000.0 );

        CHECK_EQUAL( 365.0, matrix.GetMatrixTimeWindow() );
        CHECK_EQUAL(     0, matrix.GetNumCombinations()  );
        CHECK_EQUAL(     0, matrix.GetNumGenomes()       );

        matrix.CalculateAverages( &avg_ibs, &avg_ibd );
        CHECK_CLOSE( 0.0, avg_ibs, FLT_EPSILON );
        CHECK_CLOSE( 0.0, avg_ibd, FLT_EPSILON );
    }
#endif

    TEST_FIXTURE( GeneticsFixture, TestPerformance )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ReportNodeDemographicsMalariaGeneticsTest/TestPerformance.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        std::vector<std::vector<float>> freqs_barcode;
        std::vector<std::vector<float>> freqs_drug;
        std::vector<std::vector<float>> freqs_hrp;


        for( int i = 0; i < ParasiteGenetics::CreateInstance()->GetNumBasePairs(); ++i )
        {
            std::vector<float> inner;
            inner.push_back( 0.5 );
            inner.push_back( 0.5 );
            inner.push_back( 0.0 );
            inner.push_back( 0.0 );

            freqs_barcode.push_back( inner );
        }

        //int num_genomes = 10000;
        //float sim_duration = 3650;
        //int max_infections_per_time_step = 20;

        int num_genomes = 4000;
        float sim_duration = 1096;
        int max_infections_per_time_step = 15;

        std::vector<ParasiteGenome> genome_list;
        for( int i = 0; i < num_genomes; ++i )
        {
            ParasiteGenome genome  = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                         freqs_barcode,
                                                                                                         freqs_drug,
                                                                                                         freqs_hrp );
            ParasiteGenome genome_with_roots = ParasiteGenetics::GetInstance()->CreateGenome( genome, i );

            genome_list.push_back( genome_with_roots );
        }

        Stopwatch watch ;
        watch.Start();

        int max_genomes = 0;
        int max_combos = 0;

        GenomeComboMatrix matrix( 365.0 );
        int genome_index = 0;
        for( float current_time = 0.0; current_time < sim_duration; current_time += 1.0 )
        {
            int add_num_genomes = rng.uniformZeroToN16( max_infections_per_time_step );

            for( int i = 0; i < add_num_genomes; ++i )
            {
                matrix.AddGenome( genome_list[ genome_index ], current_time );
                ++genome_index;
                if( genome_index >= genome_list.size() )
                {
                    genome_index = 0;
                }
            }

            float avg_ibs = 0.0;
            float avg_ibd = 0.0;
            matrix.CalculateAverages( &avg_ibs, &avg_ibd );

            matrix.RemoveOld( current_time );

            if( max_genomes < matrix.GetNumGenomes() )
            {
                max_genomes = matrix.GetNumGenomes();
            }
            if( max_combos < matrix.GetNumCombinations() )
            {
                max_combos = matrix.GetNumCombinations();
            }
        }
        watch.Stop();
        double ms = watch.ResultNanoseconds() / 1000000.0;
        printf("ReportNodeDemographicsMalariaGeneticsTest: TestPerformance: max_genomes=%d  max_combos=%d  duration(ms)=%f\n",max_genomes,max_combos,ms);

        // ---------------------------------------------------------------------------
        // --- On my (DanB) VM, X iterations of this loop had the following averages:
        // --- Positions/Iterations  100/50  1000/16
        // --- AVX2                   2.6s    20.3s
        // --- AVX                    2.8s    22.8s
        // --- Neither                3.6s    25.8s
        // ---------------------------------------------------------------------------

        CHECK( ms < 4500.0 ); // Test is slower when running with other tests
    }

}