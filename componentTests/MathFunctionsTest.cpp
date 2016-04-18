/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "UnitTest++.h"
#include "MathFunctions.h"

SUITE(MathFunctionsTest)
{
    TEST(TestCalcualteDistance)
    {
        double d_km_0 = Kernel::CalculateDistanceKm( 0.0, 0.0, 0.0, 0.0 );
        CHECK_EQUAL( 0.0, d_km_0 );

        d_km_0 = Kernel::CalculateDistanceKm( 30.0, 40.0, 30.0, 40.0 );
        CHECK_EQUAL( 0.0, d_km_0 );

        double d_km_1 = Kernel::CalculateDistanceKm( 0.0, 0.0, 0.0, 1.0 );
        double d_km_2 = Kernel::CalculateDistanceKm( 0.0, 0.0, 1.0, 0.0 );
        double d_km_3 = Kernel::CalculateDistanceKm( 0.0, 1.0, 0.0, 0.0 );
        double d_km_4 = Kernel::CalculateDistanceKm( 1.0, 0.0, 0.0, 0.0 );
        CHECK_CLOSE( 111.23, d_km_1, 0.01 );
        CHECK_CLOSE( 111.23, d_km_2, 0.01 );
        CHECK_CLOSE( 111.23, d_km_3, 0.01 );
        CHECK_CLOSE( 111.23, d_km_4, 0.01 );

        d_km_1 = Kernel::CalculateDistanceKm( 30.0, 40.0, 30.0, 41.0 );
        d_km_2 = Kernel::CalculateDistanceKm( 30.0, 40.0, 31.0, 40.0 );
        d_km_3 = Kernel::CalculateDistanceKm( 30.0, 41.0, 30.0, 40.0 );
        d_km_4 = Kernel::CalculateDistanceKm( 31.0, 40.0, 30.0, 40.0 );
        CHECK_CLOSE( 111.23, d_km_1, 0.01 );
        CHECK_CLOSE(  85.20, d_km_2, 0.01 );
        CHECK_CLOSE( 111.23, d_km_3, 0.01 );
        CHECK_CLOSE(  85.20, d_km_4, 0.01 );

        d_km_1 = Kernel::CalculateDistanceKm( -179.5, 40.0,  179.5, 40.0 );
        d_km_2 = Kernel::CalculateDistanceKm(  179.5, 40.0, -179.5, 40.0 );
        CHECK_CLOSE( 85.20, d_km_1, 0.01 );
        CHECK_CLOSE( 85.20, d_km_2, 0.01 );

        d_km_1 = Kernel::CalculateDistanceKm( 45.0,  88.0, 45.0,  89.0 );
        d_km_2 = Kernel::CalculateDistanceKm( 45.0, -88.0, 45.0, -89.0 );
        CHECK_CLOSE( 111.23, d_km_1, 0.01 );
        CHECK_CLOSE( 111.23, d_km_2, 0.01 );

        d_km_1 = Kernel::CalculateDistanceKm( 45.0,  89.0, 45.0,  91.0 );
        d_km_2 = Kernel::CalculateDistanceKm( 45.0, -89.0, 45.0, -91.0 );
        CHECK_CLOSE( 222.46, d_km_1, 0.01 );
        CHECK_CLOSE( 222.46, d_km_2, 0.01 );

        d_km_1 = Kernel::CalculateDistanceKm( -110.5, 45.0,  -105.0, 40.0 );
        CHECK_CLOSE( 715.69, d_km_1, 0.01 );
    }
}
