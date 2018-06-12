/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "common.h"
#include "RANDOM.h"
#include <iostream>
#include <iomanip>
#include <math.h>

SUITE(PrngTest)
{
    TEST( Test_eGauss )
    {
        RANDOMBASE* prng = new PSEUDO_DES( 42 );

        float sum = 0.0;
        float sum2 = 0.0;
        int num = 100000;
        for( int i = 0; i < num; ++i )
        {
            float val = float( prng->eGauss() );
            sum += val;
            sum2 += val*val;
            //std::stringstream ss;
            //ss << val << "\n";
            //PrintDebug( ss.str() );
        }
        float avg = sum / float( num );
        float stddev = sqrt( sum2 / float( num ) - (avg*avg) );
        CHECK_CLOSE( 0.0f, avg, 0.01f );
        CHECK_CLOSE( 1.0f, stddev, 0.01f );
    }

    TEST(TestPseudoDes)
    {
        static uint32_t baseline[20] = {
            0xC54A92D9, 0x910ABB3E, 0x6581F580, 0xA4C515EB,
            0x68F56A97, 0x990AC362, 0xF40EDF6D, 0x32E1E146,
            0xDB165164, 0x7D1198D5, 0xA4B6860B, 0xD55BE460,
            0x607CDC4C, 0x2EEE692C, 0x870408D3, 0x1C677DB0,
            0xE162A0B3, 0x8E0C8452, 0x7A4D3E26, 0x51293548
        };
        RANDOMBASE* prng = new PSEUDO_DES(42);
        std::cout << "-----====##### TestPseudoDes #####=====-----" << std::endl;
        // Display hexadecimal in uppercase right aligned with '0' for fill.
        std::cout << std::hex << std::uppercase << std::right << std::setfill('0');
        size_t iCompare = 0;
        for (size_t outer = 0; outer < 20; ++outer)
        {
            uint32_t expected = baseline[iCompare++];
            uint32_t actual = prng->ul();
            // setw() is only good for the next output...
            std::cout << "expected: 0x" << std::setw(8) << expected << ", actual: 0x" << std::setw(8) << actual << std::endl;
            CHECK_EQUAL(expected, actual);
            for (size_t inner = 0; inner < 1023; ++inner)
            {
                /* actual = */ prng->ul();
            }
        }
        std::cout << std::endl;
    }

    TEST(TestAesCounter)
    {
        static uint32_t baseline[20] = {
            0x32CF19A7, 0x5959DB43, 0x066D5837, 0x947278BF,
            0x01AB14DF, 0x59346DA2, 0xDE760396, 0x67200715,
            0xE02F9E28, 0xE18F2907, 0xD62200CB, 0x0C441867,
            0x9DAE6A22, 0x0D350267, 0xEB73D975, 0xD96EDD8C,
            0xD165DDE4, 0x261C7D76, 0x8AC4F6B1, 0x77695656
        };
        RANDOMBASE* prng = new AES_COUNTER(42, 0);
        std::cout << "-----====##### TestAesCounter #####=====-----" << std::endl;
        // Display hexadecimal in uppercase right aligned with '0' for fill.
        std::cout << std::hex << std::uppercase << std::right << std::setfill('0');
        size_t iCompare = 0;
        for (size_t outer = 0; outer < 20; ++outer)
        {
            uint32_t expected = baseline[iCompare++];
            uint32_t actual = prng->ul();
            // setw() is only good for the next output...
            std::cout << "expected: 0x" << std::setw(8) << expected << ", actual: 0x" << std::setw(8) << actual << std::endl;
            CHECK_EQUAL(expected, actual);
            for (size_t inner = 0; inner < 1023; ++inner)
            {
                /* actual = */ prng->ul();
            }
        }
        std::cout << std::endl;
    }

    TEST( TestRandomRound )
    {
        RANDOMBASE* prng = new PSEUDO_DES( 42 );

        for( float inc=0.0; inc < 1.0 ; inc += 0.1f )
        {
            float val = 5.0f + inc;

            uint32_t total_count = 1000000;
            uint32_t count_5 = 0;
            uint32_t count_6 = 0;
            for( uint32_t i = 0; i < total_count; ++i )
            {
                uint32_t i_val = prng->randomRound( val );

                if( i_val == 5 )
                {
                    ++count_5;
                }
                else if( i_val == 6 )
                {
                    ++count_6;
                }
                else
                {
                    CHECK( false );
                }
            }
            float percent_5 = float( count_5 ) / float( total_count );
            float percent_6 = float( count_6 ) / float( total_count );

            CHECK_CLOSE( inc, percent_6, 0.001 );
            CHECK_CLOSE( 1.0, percent_5 + percent_6, 0.001 );
        }
    }

    TEST( TestMultinomialApprox )
    {
        RANDOMBASE* prng = new PSEUDO_DES( 42 );

        uint64_t N = 100000;
        uint64_t num_samples = 10000;

        for( float total_fraction = 0.9f ; total_fraction >= 0.1f ; total_fraction -= 0.1f )
        {
            // assuming total of fractions = 1 before multiplying times total_fraction
            std::vector<float> fractions;
            fractions.push_back( 0.1f * total_fraction );
            fractions.push_back( 0.2f * total_fraction );
            fractions.push_back( 0.3f * total_fraction );
            fractions.push_back( 0.4f * total_fraction );

            std::vector<uint64_t> totals( fractions.size(), 0 );
            for( uint32_t i = 0; i < num_samples; ++i )
            {
                std::vector<uint64_t> subsets = prng->multinomial_approx( N, fractions );

                uint64_t total_of_subsets = 0;
                for( int j = 0; j < subsets.size(); ++j )
                {
                    totals[ j ] += subsets[ j ];
                    total_of_subsets += subsets[ j ];
                }
                float percent_N = float( total_of_subsets ) / float( N );
                //printf("total_fraction=%f  N=%lld  total_of_subsets=%lld  percent_N=%f\n",total_fraction,N,total_of_subsets, percent_N);
                CHECK_CLOSE( total_fraction, percent_N, 0.01 );
            }
            for( int i = 0; i < totals.size(); ++i )
            {
                double percent = double( totals[ i ] ) / double( num_samples*N );
                //printf("totals[%d]=%lld percent=%f fractions[%d]=%f\n",i,totals[i],percent,i,fractions[i]);
                CHECK_CLOSE( fractions[ i ], percent, 0.0001 );
            }
        }
    }
}