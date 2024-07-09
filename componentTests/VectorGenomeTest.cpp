
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "VectorGenome.h"
#include <bitset>

using namespace Kernel;

SUITE( VectorGenomeTest )
{
    struct VectorGenomeFixture
    {
        VectorGenomeFixture()
        {
        }

        ~VectorGenomeFixture()
        {
        }
    };

    TEST_FIXTURE( VectorGenomeFixture, TestGetSet )
    {
        VectorGamete gamete_f;
        gamete_f.SetLocus( 0, 0 );
        gamete_f.SetLocus( 1, 2 );
        gamete_f.SetLocus( 2, 3 );
        gamete_f.SetLocus( 3, 4 );
        gamete_f.SetLocus( 4, 5 );
        gamete_f.SetWolbachia( VectorWolbachia::VECTOR_WOLBACHIA_B );
        gamete_f.ClearMicrosporidia();

        VectorGamete gamete_m;
        gamete_m.SetLocus( 0, 4 );
        gamete_m.SetLocus( 1, 4 );
        gamete_m.SetLocus( 2, 3 );
        gamete_m.SetLocus( 3, 2 );
        gamete_m.SetLocus( 4, 1 );
        gamete_m.SetWolbachia( VectorWolbachia::VECTOR_WOLBACHIA_B );
        gamete_m.ClearMicrosporidia();

        VectorGenome genome( gamete_f, gamete_m );

        std::pair<uint8_t,uint8_t> allele_index_pair = genome.GetLocus( 0 );
        CHECK_EQUAL( 0, allele_index_pair.first  );
        CHECK_EQUAL( 4, allele_index_pair.second );

        allele_index_pair = genome.GetLocus( 1 );
        CHECK_EQUAL( 2, allele_index_pair.first );
        CHECK_EQUAL( 4, allele_index_pair.second );

        allele_index_pair = genome.GetLocus( 2 );
        CHECK_EQUAL( 3, allele_index_pair.first );
        CHECK_EQUAL( 3, allele_index_pair.second );

        allele_index_pair = genome.GetLocus( 3 );
        CHECK_EQUAL( 4, allele_index_pair.first );
        CHECK_EQUAL( 2, allele_index_pair.second );

        allele_index_pair = genome.GetLocus( 4 );
        CHECK_EQUAL( 5, allele_index_pair.first );
        CHECK_EQUAL( 1, allele_index_pair.second );

        VectorWolbachia::Enum wolb = genome.GetWolbachia();
        CHECK_EQUAL( VectorWolbachia::VECTOR_WOLBACHIA_B, wolb );

        bool has_microsporidia = genome.HasMicrosporidia();
        CHECK( !has_microsporidia );

        CHECK_EQUAL( VectorGender::VECTOR_MALE, genome.GetGender() );

        // 10 000 000 000 000 000 001 010 011 100 100 10 000 000 000 000 000 101 100 011 010 000
        std::bitset<64> bs = genome.GetBits();
        CHECK_EQUAL( "1000000000000000000101001110010010000000000000000101100011010000", bs.to_string() );

        VectorGenome genome2;
        CHECK( genome != genome2 );
        genome2.SetLocus( 0, 0, 4 );
        genome2.SetLocus( 1, 2, 4 );
        genome2.SetLocus( 2, 3, 3 );
        genome2.SetLocus( 3, 4, 2 );
        genome2.SetLocus( 4, 5, 1 );
        genome2.SetWolbachia( VectorWolbachia::VECTOR_WOLBACHIA_B );
        genome2.ClearMicrosporidia();

        CHECK( genome == genome2 );

        bs = genome2.GetBits();
        CHECK_EQUAL( "1000000000000000000101001110010010000000000000000101100011010000", bs.to_string() );
        CHECK_EQUAL( VectorGender::VECTOR_MALE, genome2.GetGender() );

        genome2.SetMicrosporidiaStrain( 1 );

        CHECK( genome2.HasMicrosporidia() );
        bs = genome2.GetBits();
        CHECK_EQUAL( "1000100000000000000101001110010010001000000000000101100011010000", bs.to_string() );
        CHECK_EQUAL( 1, genome2.GetMicrosporidiaStrainIndex() );

        VectorGenome genome3( genome2.GetBits() );

        CHECK( genome != genome3 );

        genome.SetMicrosporidiaStrain( 1 );

        CHECK( genome == genome3 );
    }
}
