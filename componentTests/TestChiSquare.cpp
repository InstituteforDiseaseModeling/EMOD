/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "ChiSquare.h"

SUITE( TestChiSquare )
{
    TEST( TestReduceDataForChiSquareStatistic )
    {
        std::vector<float> exp_in;
        std::vector<float> act_in;
        std::vector<float> exp_out;
        std::vector<float> act_out;

        float e1[] = { 5.0, 6.0, 7.0, 8.0, 9.0 };
        float a1[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
        exp_in.assign( e1, e1 + 5 );
        act_in.assign( a1, a1 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        CHECK_ARRAY_CLOSE( e1, exp_out, 5, 0.00001 );
        CHECK_ARRAY_CLOSE( a1, act_out, 5, 0.00001 );

        float e2[] = { 0.0, 6.0, 7.0, 8.0, 9.0 };
        float a2[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
        exp_in.assign( e2, e2 + 5 );
        act_in.assign( a2, a2 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        float t2_out[] = { 6.0, 7.0, 8.0, 9.0 };
        float a2_out[] = { 3.0, 3.0, 4.0, 5.0 };
        CHECK_ARRAY_CLOSE( t2_out, exp_out, 4, 0.00001 );
        CHECK_ARRAY_CLOSE( a2_out, act_out, 4, 0.00001 );

        float e3[] = { 5.0, 6.0, 7.0, 8.0, 0.0 };
        float a3[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
        exp_in.assign( e3, e3 + 5 );
        act_in.assign( a3, a3 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        float t3_out[] = { 5.0, 6.0, 7.0, 8.0 };
        float a3_out[] = { 1.0, 2.0, 3.0, 9.0 };
        CHECK_ARRAY_CLOSE( t3_out, exp_out, 4, 0.00001 );
        CHECK_ARRAY_CLOSE( a3_out, act_out, 4, 0.00001 );

        float e4[] = { 5.0, 6.0, 0.0, 8.0, 9.0 };
        float a4[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
        exp_in.assign( e4, e4 + 5 );
        act_in.assign( a4, a4 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        float t4_out[] = { 5.0, 6.0, 8.0, 9.0 };
        float a4_out[] = { 1.0, 2.0, 7.0, 5.0 };
        CHECK_ARRAY_CLOSE( t4_out, exp_out, 4, 0.00001 );
        CHECK_ARRAY_CLOSE( a4_out, act_out, 4, 0.00001 );

        float e5[] = { 0.0, 6.0, 7.0, 8.0, 0.0 };
        float a5[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
        exp_in.assign( e5, e5 + 5 );
        act_in.assign( a5, a5 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        float t5_out[] = { 6.0, 7.0, 8.0 };
        float a5_out[] = { 3.0, 3.0, 9.0 };
        CHECK_ARRAY_CLOSE( t5_out, exp_out, int(exp_out.size()), 0.00001 );
        CHECK_ARRAY_CLOSE( a5_out, act_out, int(act_out.size()), 0.00001 );

        float e6[] = { 0.0, 0.0, 7.0, 8.0, 9.0 };
        float a6[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
        exp_in.assign( e6, e6 + 5 );
        act_in.assign( a6, a6 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        float t6_out[] = { 7.0, 8.0, 9.0 };
        float a6_out[] = { 6.0, 4.0, 5.0 };
        CHECK_ARRAY_CLOSE( t6_out, exp_out, int(exp_out.size()), 0.00001 );
        CHECK_ARRAY_CLOSE( a6_out, act_out, int(act_out.size()), 0.00001 );

        float e7[] = { 5.0, 6.0, 7.0, 0.0, 0.0 };
        float a7[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
        exp_in.assign( e7, e7 + 5 );
        act_in.assign( a7, a7 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        float t7_out[] = { 5.0, 6.0, 7.0 };
        float a7_out[] = { 1.0, 2.0,12.0 };
        CHECK_ARRAY_CLOSE( t7_out, exp_out, int(exp_out.size()), 0.00001 );
        CHECK_ARRAY_CLOSE( a7_out, act_out, int(act_out.size()), 0.00001 );

        float e8[] = { 5.0, 6.0, 0.0, 0.0, 9.0 };
        float a8[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
        exp_in.assign( e8, e8 + 5 );
        act_in.assign( a8, a8 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        float t8_out[] = { 5.0, 6.0, 9.0 };
        float a8_out[] = { 1.0, 2.0, 12.0 };
        CHECK_ARRAY_CLOSE( t8_out, exp_out, int(exp_out.size()), 0.00001 );
        CHECK_ARRAY_CLOSE( a8_out, act_out, int(act_out.size()), 0.00001 );

        float e9[] = { 1.1f, 4.0f, 2.2f, 3.0f, 9.0f };
        float a9[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
        exp_in.assign( e9, e9 + 5 );
        act_in.assign( a9, a9 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        float t9_out[] = { 5.1f, 5.2f, 9.0f };
        float a9_out[] = { 3.0f, 7.0f, 5.0f };
        CHECK_ARRAY_CLOSE( t9_out, exp_out, int(exp_out.size()), 0.00001 );
        CHECK_ARRAY_CLOSE( a9_out, act_out, int(act_out.size()), 0.00001 );

        float e10[] = { 8.0f, 9.0f, 1.1f, 1.2f, 3.3f };
        float a10[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
        exp_in.assign( e10, e10 + 5 );
        act_in.assign( a10, a10 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        float t10_out[] = { 8.0f, 9.0f, 5.6f };
        float a10_out[] = { 1.0f, 2.0f, 12.0f };
        CHECK_ARRAY_CLOSE( t10_out, exp_out, int(exp_out.size()), 0.00001 );
        CHECK_ARRAY_CLOSE( a10_out, act_out, int(act_out.size()), 0.00001 );

        float e11[] = { 8.0f, 9.0f, 1.1f, 1.2f, 1.3f };
        float a11[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
        exp_in.assign( e11, e11 + 5 );
        act_in.assign( a11, a11 + 5 );
        ChiSquare::ReduceDataForChiSquareStatistic( 5.0f, exp_in, act_in, exp_out, act_out );
        float t11_out[] = { 8.0f, 12.6f };
        float a11_out[] = { 1.0f, 14.0f };
        CHECK_ARRAY_CLOSE( t11_out, exp_out, int(exp_out.size()), 0.00001 );
        CHECK_ARRAY_CLOSE( a11_out, act_out, int(act_out.size()), 0.00001 );
    }
}
