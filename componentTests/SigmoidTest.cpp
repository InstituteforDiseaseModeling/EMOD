/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "MathFunctions.h"

using namespace std; 
using namespace Kernel; 

SUITE(SigmoidTest)
{
    TEST(Test1)
    {
        std::cout << "Testing Sigmoid functions." << std::endl;

        // Start of by checking that basic_sigmoid does what is expected. The reference values are 
        // basically just taken by duplicating the math in the code as of this writing.
        auto val = Sigmoid::basic_sigmoid(); // defaults
        CHECK( val == 0 );
        val = Sigmoid::basic_sigmoid( 100.0, 1.0 ); // defaults
        CHECK( val == (1.0/101.0) );
        val = Sigmoid::basic_sigmoid( 100.0, 50.0 ); // defaults
        CHECK( val == (50.0/150.0) );
        val = Sigmoid::basic_sigmoid( 100.0, 100.0 ); // defaults
        CHECK( val == (100.0/200.0) );
        val = Sigmoid::basic_sigmoid( 1.0, 100.0 ); // defaults
        CHECK( val == (100.0/101.0) );

        // Now test the 3-parameter sigmoid function that has variable width (but not height).
        // Negative test
        try 
        {
            Sigmoid::variableWidthSigmoid( FLT_MAX, FLT_MAX, -0.0001f );
        }
        catch( OutOfRangeException &e )
        {
            std::string msg = e.GetMsg();
            std::string exp_msg = "invwidth" ;
            CHECK( msg.find( exp_msg ) != string::npos );
        }

        float last_value = -1;
        float threshold = 2.0f;
        for( float input = -100.0f /* start really low and negative */; 
                   input < 4000.0f /* end really high */;
                   input += 3.33f /* pick increment that gives us plenty of decimal values */ )
        {
            auto val = Sigmoid::variableWidthSigmoid( input, threshold /*mid*/, 0.001f /* inverse width */);
            //std::cout << "input = " << input << ", val = " << val << std::endl;
            // range check
            CHECK( val > 0.0f && val < 1.0f );
            // monotonicity check
            CHECK( val > last_value );
            // lower/upper half check
            if( input < threshold )
            {
                CHECK( val < 0.5f );
            }
            else if( input > threshold )
            {
                CHECK( val > 0.5f );
            }
            else
            {
                CHECK( val == 0.5f );
            }
            last_value = val;
        }

        // Now test the 4-parameter sigmoid function that is actually used everywhere.
        // negative
        try 
        {
            Sigmoid::variableWidthAndHeightSigmoid( FLT_MAX, FLT_MAX, -FLT_MAX, 44.4f, 44.3f );
        }
        catch( ConfigurationRangeException &e )
        {
            std::string msg = e.GetMsg();
            std::string exp_msg = "max_val - min_val" ;
            CHECK( msg.find( exp_msg ) != string::npos );
        }
        // extremity tests.
        val = Sigmoid::variableWidthAndHeightSigmoid( 0.0f /* "really low value" */, 1950.0f, 1.0f, 0.1f, 0.9f);
        CHECK( val == 0.1f );
        val = Sigmoid::variableWidthAndHeightSigmoid( 20000.0f /* "really high value"*/, 1950.0f, 1.0f, 0.1f, 0.9f);
        CHECK( val == 0.9f );
        // mid-point
        val = Sigmoid::variableWidthAndHeightSigmoid( 1950.0f /*==mid*/, 1950.0f, 1.0f, 0.1f, 0.9f);
        CHECK_CLOSE( 0.5f, val, FLT_EPSILON ); // note the 'epsilon' delta test. Exact match fails.

        // let's look at the results of picking some samples across a certain parameterization
        // Pick values that approach midpoint of 'left/lower' side
        float inputs[] = { 1990.0f, 1995.0f, 1999.0f, 1999.5f, 1999.9f, 1999.99f, 1999.999f };
        // outputs were picked from actual calculations, not theory or independent calculation
        float expected[] = { 4.24835e-018f, 2.06115e-009f, 0.0179862f, 0.119203f, 0.401336f, 0.489992f, 0.499023f };

        // note that loop isn't great in case of failure which will show line number but not idx.
        for( int idx=0; idx<sizeof(inputs)/sizeof(float); idx++ )
        {
            auto val = Sigmoid::variableWidthAndHeightSigmoid( inputs[idx], 2000.0f, 4.0f, 0.0f, 1.0f);
            CHECK_CLOSE( expected[idx], val, 1e-5f );
        }

        // let's slow sweep over a lot of values and make sure we're monotonic and never exceed bounds.
        // Pick range that includes positive and negative.
        // Pick "really shallow" slope
        last_value = -1;
        float mid = 2000.0f;
        for( float input = -100.0f /* start really low and negative */; 
                   input < 4000.0f /* end really high */;
                   input += 3.33f /* pick increment that gives us plenty of decimal values */ )
        {
            auto val = Sigmoid::variableWidthAndHeightSigmoid( input, mid /*mid*/, 0.0001f /*slope*/, -0.5f, 0.5f);
            // range check
            CHECK( val > -0.5f && val < 0.5f );
            // monotonicity check
            CHECK( val > last_value );
            // lower/upper half check
            if( input < mid )
            {
                CHECK( val < 0.0f );
            }
            else if( input > mid )
            {
                CHECK( val > 0.0f );
            }
            else
            {
                CHECK( val == 0.0f );
            }
            last_value = val;
        }
    }
}
