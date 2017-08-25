/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "RANDOM.h"
#include <iostream>
#include <iomanip>

SUITE(PrngTest)
{
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
}
