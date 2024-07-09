
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "VectorGamete.h"
#include <bitset>

using namespace Kernel;

SUITE( VectorGameteTest )
{
    struct VectorGameteFixture
    {
        VectorGameteFixture()
        {
        }

        ~VectorGameteFixture()
        {
        }
    };

    TEST_FIXTURE( VectorGameteFixture, TestGetSet )
    {
        VectorGamete gamete;
        gamete.SetLocus( 0, 3 );
        gamete.SetLocus( 1, 2 );
        gamete.SetLocus( 2, 7 );
        gamete.SetLocus( 3, 6 );
        gamete.SetLocus( 4, 5 );
        gamete.SetLocus( 5, 4 );
        gamete.SetLocus( 6, 3 );
        gamete.SetLocus( 7, 2 );
        gamete.SetLocus( 8, 1 );
        gamete.SetWolbachia( VectorWolbachia::VECTOR_WOLBACHIA_B );

        CHECK_EQUAL( 3, gamete.GetLocus( 0 ) );
        CHECK_EQUAL( 2, gamete.GetLocus( 1 ) );
        CHECK_EQUAL( 7, gamete.GetLocus( 2 ) );
        CHECK_EQUAL( 6, gamete.GetLocus( 3 ) );
        CHECK_EQUAL( 5, gamete.GetLocus( 4 ) );
        CHECK_EQUAL( 4, gamete.GetLocus( 5 ) );
        CHECK_EQUAL( 3, gamete.GetLocus( 6 ) );
        CHECK_EQUAL( 2, gamete.GetLocus( 7 ) );
        CHECK_EQUAL( 1, gamete.GetLocus( 8 ) );
        CHECK_EQUAL( VectorWolbachia::VECTOR_WOLBACHIA_B, gamete.GetWolbachia() );
        CHECK_EQUAL( false, gamete.HasMicrosporidia() );

        // 10 000 001 010 011 100 101 110 111 010 011
        CHECK_EQUAL( 2169396691, gamete.GetBits() );
        std::bitset<32> bs = gamete.GetBits();
        CHECK_EQUAL( "10000001010011100101110111010011", bs.to_string() );

        VectorGamete gamete2( 2169396691 );

        CHECK( gamete == gamete2 );

        gamete.SetLocus( 3, 0 );
        gamete.SetLocus( 6, 7 );
        gamete.SetWolbachia( VectorWolbachia::VECTOR_WOLBACHIA_A );

        CHECK_EQUAL( 3, gamete.GetLocus( 0 ) );
        CHECK_EQUAL( 2, gamete.GetLocus( 1 ) );
        CHECK_EQUAL( 7, gamete.GetLocus( 2 ) );
        CHECK_EQUAL( 0, gamete.GetLocus( 3 ) );
        CHECK_EQUAL( 5, gamete.GetLocus( 4 ) );
        CHECK_EQUAL( 4, gamete.GetLocus( 5 ) );
        CHECK_EQUAL( 7, gamete.GetLocus( 6 ) );
        CHECK_EQUAL( 2, gamete.GetLocus( 7 ) );
        CHECK_EQUAL( 1, gamete.GetLocus( 8 ) );
        CHECK_EQUAL( VectorWolbachia::VECTOR_WOLBACHIA_A, gamete.GetWolbachia() );
        CHECK_EQUAL( false, gamete.HasMicrosporidia() );

        bs = gamete.GetBits();
        CHECK_EQUAL( "01000001010111100101000111010011", bs.to_string() );

        gamete.SetMicrosporidiaStrain( 2 );

        bs = gamete.GetBits();
        CHECK_EQUAL( "01010001010111100101000111010011", bs.to_string() );

        CHECK_EQUAL( 3, gamete.GetLocus( 0 ) );
        CHECK_EQUAL( 2, gamete.GetLocus( 1 ) );
        CHECK_EQUAL( 7, gamete.GetLocus( 2 ) );
        CHECK_EQUAL( 0, gamete.GetLocus( 3 ) );
        CHECK_EQUAL( 5, gamete.GetLocus( 4 ) );
        CHECK_EQUAL( 4, gamete.GetLocus( 5 ) );
        CHECK_EQUAL( 7, gamete.GetLocus( 6 ) );
        CHECK_EQUAL( 2, gamete.GetLocus( 7 ) );
        CHECK_EQUAL( 1, gamete.GetLocus( 8 ) );
        CHECK_EQUAL( VectorWolbachia::VECTOR_WOLBACHIA_A, gamete.GetWolbachia() );
        CHECK_EQUAL( true, gamete.HasMicrosporidia() );
        CHECK_EQUAL( 2, gamete.GetMicrosporidiaStrainIndex() );
    }

    TEST_FIXTURE( VectorGameteFixture, TestConvert )
    {
        VectorGamete vg_f;
        vg_f.SetLocus( 0, 1 );
        vg_f.SetLocus( 1, 2 );
        vg_f.SetLocus( 2, 3 );
        vg_f.SetLocus( 3, 4 );
        vg_f.SetLocus( 4, 5 );

        VectorGamete vg_m;
        vg_m.SetLocus( 0, 5 );
        vg_m.SetLocus( 1, 4 );
        vg_m.SetLocus( 2, 3 );
        vg_m.SetLocus( 3, 2 );
        vg_m.SetLocus( 4, 1 );

        CHECK( vg_f == vg_f );
        CHECK( vg_m == vg_m );
        CHECK( vg_f != vg_m );
        CHECK( vg_m != vg_f );

        VectorGameteBitPair_t bit_pair = VectorGamete::Convert( vg_f, vg_m );
        std::pair<VectorGamete, VectorGamete> vg_pair = VectorGamete::Convert( bit_pair );
        CHECK( vg_f == vg_pair.first );
        CHECK( vg_m == vg_pair.second );
    }

    TEST_FIXTURE( VectorGameteFixture, TestInvalids )
    {
        VectorGamete gamete;

        // Test set invalid gene index
        try
        {
            gamete.SetLocus( 16, 0 ); // MAX_GENES = 9 < 16
            CHECK( false );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( true );
        }

        // Test set invalid allele index
        try
        {
            gamete.SetLocus( 0, 16 ); // MAX_ALLELES = 8 < 16
            CHECK( false );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( true );
        }

        // Test get invalid gene index
        try
        {
            gamete.GetLocus( 16 ); // MAX_GENES = 9 < 16
            CHECK( false );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( true );
        }
    }
}
