/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <math.h>
#include "Exceptions.h"
#include "SimulationEnums.h" // to get DistributionFunction enum. Don't want utils reaching into Eradication though. TBD!!!

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

struct Sigmoid
{
    inline static double basic_sigmoid ( double threshold = 100.0, double variable = 0.0 )
    {
        return (variable > 0) ? (variable / (threshold + variable)) : 0.0;
    }

    inline static float variableWidthSigmoid( float variable, float threshold, float invwidth )
    {
        if ( invwidth > 0 )
        {
            return 1.0f / ( 1.0f + exp( (threshold-variable) / (threshold/invwidth) ) );
        }
        else
        {
            throw Kernel::OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "invwidth", invwidth, 0 );
        }
    }

    inline static float variableWidthAndHeightSigmoid( float variable, float center, float rate, float min_val, float max_val)
    {
        // max_val must be >= min_val, however rate can be negative.
        // A positive (negative) rate creates a sigmoid that increases (decreases) with variable
        if ( max_val - min_val >= 0 )
        {
            return min_val + (max_val-min_val) / ( 1 + exp(-rate * (variable-center)) );
        }
        else
        {
            throw Kernel::ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "max_val - min_val", max_val - min_val, 0);
        }
    }
};

namespace Kernel {

class Probability
{
    public:
        static Probability * getInstance()
        {
            if( _instance == NULL )
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
}
