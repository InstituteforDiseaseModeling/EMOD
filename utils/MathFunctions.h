/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <math.h>
#include "Exceptions.h"
#include "EnumSupport.h"

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

    // ENUM defs for INCUBATION_DISTRIBUTION, INFECTIOUS_DISTRIBUTION
    ENUM_DEFINE(DistributionFunction, 
        ENUM_VALUE_SPEC(NOT_INITIALIZED                                     , -1)
        ENUM_VALUE_SPEC(FIXED_DURATION                                      , 0)
        ENUM_VALUE_SPEC(UNIFORM_DURATION                                    , 1)
        ENUM_VALUE_SPEC(GAUSSIAN_DURATION                                   , 2)
        ENUM_VALUE_SPEC(EXPONENTIAL_DURATION                                , 3)
        ENUM_VALUE_SPEC(POISSON_DURATION                                    , 4)
        ENUM_VALUE_SPEC(LOG_NORMAL_DURATION                                 , 5)
        ENUM_VALUE_SPEC(BIMODAL_DURATION                                    , 6)
        ENUM_VALUE_SPEC(PIECEWISE_CONSTANT                                  , 7)
        ENUM_VALUE_SPEC(PIECEWISE_LINEAR                                    , 8)
        ENUM_VALUE_SPEC(WEIBULL_DURATION                                    , 9)
        )

class Probability
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

        double Probability::fromDistribution(Kernel::DistributionFunction::Enum distribution_flag, double param1, double param2 = 0.0, double default_value = 0.0);

    protected:

        Probability()
        {
        }

        static Probability * _instance;
};

#define LOG_2 0.6931472f

    // calculate the great-circle distance between two points along the surface a spherical earth in kilometers
    double IDMAPI CalculateDistanceKm( double lon_1_deg, double lat_1_deg, double lon_2_deg, double lat_2_deg );
}
