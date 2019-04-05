/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <math.h>
#include "Exceptions.h"
#include "EnumSupport.h"
#include "IdmApi.h"
#include "Types.h"

struct Gamma
{
    inline static double WindschitlApproximation ( double z )
    {
        if (z < 0)
        {
            //LOG_WARN_F("GammaApprox(z) is a terrible approximation with z<0 (z=%f).\n",z);
            return 0;
        }
        if (z == 0)
        {
            //LOG_WARN_F("GammaApprox(z) called with z=0.  Returning DBL_MAX instead of infinity.\n");
            return FLT_MAX;
        }
        return (2.5066282746310002 * sqrt(1.0/z) * pow((z/2.718281828459045) * sqrt(z*sinh(1/z) + 1/(810*pow(z,6))), z));
    }
};

namespace Kernel {
    class RANDOMBASE;

    // ENUM defs for INCUBATION_DISTRIBUTION, INFECTIOUS_DISTRIBUTION
    ENUM_DEFINE(DistributionFunction, 
        ENUM_VALUE_SPEC( CONSTANT_DISTRIBUTION                               , 0 )
        ENUM_VALUE_SPEC( UNIFORM_DISTRIBUTION                                , 1 )
        ENUM_VALUE_SPEC( GAUSSIAN_DISTRIBUTION                               , 2 )
        ENUM_VALUE_SPEC( EXPONENTIAL_DISTRIBUTION                            , 3 )
        ENUM_VALUE_SPEC( POISSON_DISTRIBUTION                                , 4 )
        ENUM_VALUE_SPEC( LOG_NORMAL_DISTRIBUTION                             , 5 )
        ENUM_VALUE_SPEC( DUAL_CONSTANT_DISTRIBUTION                          , 6 )
//        ENUM_VALUE_SPEC( PIECEWISE_CONSTANT                                  , 7 )	// Disable these distributions, but leave index as is (see demographics)
//        ENUM_VALUE_SPEC( PIECEWISE_LINEAR                                    , 8 )
        ENUM_VALUE_SPEC( WEIBULL_DISTRIBUTION                                , 9 )
        ENUM_VALUE_SPEC( DUAL_EXPONENTIAL_DISTRIBUTION                       ,10 )
        )

class IDMAPI Probability
{
    public:
        static Probability * getInstance()
        {
            if( _instance == nullptr )
            {
                _instance = new Probability();
            }
            return _instance;
        }

        double Probability::fromDistribution( Kernel::RANDOMBASE* pRNG,
                                              Kernel::DistributionFunction::Enum distribution_flag,
                                              double param1, 
                                              double param2 = 0.0, 
                                              double param3 = 0.0,
                                              double default_value = 0.0);

    protected:

        Probability()
        {
        }

        static Probability * _instance;
};

#define LOG_2 0.6931472f

    // calculate the great-circle distance between two points along the surface a spherical earth in kilometers
    double IDMAPI CalculateDistanceKm( double lon_1_deg, double lat_1_deg, double lon_2_deg, double lat_2_deg );

    float IDMAPI NTimeStepProbability( NonNegativeFloat PerTimeStepProbability, float dt);
}
