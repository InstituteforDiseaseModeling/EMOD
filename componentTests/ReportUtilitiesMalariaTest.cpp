
#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"

#include "Exceptions.h"
#include "JsonObject.h"
#include "Serializer.h"
#include "ReportUtilitiesMalaria.h"
#include "RandomFake.h"

using namespace Kernel;

SUITE( ReportUtilitiesMalariaTest )
{
    TEST( TestNASBADensityWithUncertainty )
    {
        RandomFake ran_fake;

        // Test a very small input density just returns zero
        float val = float( ReportUtilitiesMalaria::NASBADensityWithUncertainty( &ran_fake, 0.00001f ) );
        CHECK_EQUAL( 0.0, val );

        // Test that we have a minimum boundary that ends up return zero
        std::vector<uint32_t> ul_bits;
        ul_bits.push_back( 1578186 );
        ul_bits.push_back( -1470711828 );
        ul_bits.push_back( 1104454382 );
        ran_fake.SetUL( ul_bits ); // eGauss()=-3.336378
        val = float(ReportUtilitiesMalaria::NASBADensityWithUncertainty( &ran_fake, 0.00011f ));
        CHECK_EQUAL( 0.0, val );

        // Test that we have a maximum boundary
        ul_bits.clear();
        ul_bits.push_back( 9074125 );
        ul_bits.push_back( -1895856362 );
        ul_bits.push_back( -601877168 );
        ran_fake.SetUL( ul_bits ); // eGauss()=3.464311
        val = float(ReportUtilitiesMalaria::NASBADensityWithUncertainty( &ran_fake, 100000.0 ));
        CHECK_CLOSE( 2852852.7, val, 0.1 );

        // Test happy path
        ran_fake.SetUL(0); // eGauss()=0.56399559975
        val = float(ReportUtilitiesMalaria::NASBADensityWithUncertainty( &ran_fake, 10.0f ));
        CHECK_CLOSE( 25.53527, val, 0.00001 );
    }
}
