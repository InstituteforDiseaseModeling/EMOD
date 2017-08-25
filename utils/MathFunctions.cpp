/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include <stdafx.h>

#include "MathFunctions.h"
#include "Environment.h"
#include "RANDOM.h"
#include "Log.h"
#include "Debug.h"


SETUP_LOGGING( "MathFunctions" )

namespace Kernel 
{

    Probability * Probability::_instance = nullptr;
    double Probability::fromDistribution(DistributionFunction::Enum distribution_flag, double param1, double param2, double param3, double default_value)
    {
        //std::cout << __FUNCTION__ << " called with distribution = " << distribution_flag << std::endl;
        double value;
        auto rng = Environment::getInstance()->RNG;
        release_assert( rng );

        switch (distribution_flag)
        {

        case DistributionFunction::FIXED_DURATION:
            // param1 is the constant value
            value = param1; 
            break;

        case DistributionFunction::UNIFORM_DURATION:
            // uniformly distributed between param1 (min) and param2 (max)
            if ( param1 > param2 )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "param1 (UNIFORM_DURATION:min)", param1, "param2 (UNIFORM_DURATION:max)", param2 );
            }
            value = param1 + rng->e() * (param2 - param1);
            break;

        case DistributionFunction::GAUSSIAN_DURATION:
            // gaussian distribution with param1 (mean) and param2 (width)
            // negative values in PDF are truncated at 0

            if ( param2 <= 0 )
            {
                throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "param2 (GAUSSIAN_DURATION:width)", param2, 0.0f );
            }
            value = rng->eGauss() * param2 + param1;
            if (value < 0) {  value = 0; }
            break;

        case DistributionFunction::EXPONENTIAL_DURATION:
            // exponential distribution with param1 (decay length)
            if ( param1 <=0 )
            {
                throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "param1 (EXPONENTIAL_DURATION:decay length)", param1, 0 );
            }
            value = rng->expdist(param1);
            break;

        case DistributionFunction::POISSON_DURATION:
            // poisson distribution with param1 (mean)
            if ( param1 <=0 )
            {
                throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "param1 (POISSON_DURATION:mean)", param1, 0 );
            }
            value = rng->Poisson(param1);
            break;

        case DistributionFunction::LOG_NORMAL_DURATION:
            // log normal distribution with param1 (mean) and param2 (log-width)
            if (param1 <= 0 || param2 <=0)
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "param1 (LOG_NORMAL_DURATION:mean)", param1, "param2 (LOG_NORMAL_DURATION:log-width)", param2 );
            }
            value = exp( log(param1) + rng->eGauss() * param2 );
            break;

        case DistributionFunction::BIMODAL_DURATION:
            // bimodal distribution with param1 (fraction with non-unity value) and param2 (multiplier for non-unity fraction)
            // a third input parameter could give flexibility to specify the value for both parts of a bimodal distribution, 
            // but this should work fine for a multiplier, e.g. 1% immune-deficient individuals with 6% of the immune protection as normal.
            if ( param1 < 0 || param1 > 1 || param2 < 0 )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "distribution_flag", "BIMODAL_DISTRIBUTION", "param1", boost::lexical_cast<std::string>(param1).c_str() );
            }
            value = (rng->e() < param1 ) ? param2 : 1;
            break;

        case DistributionFunction::WEIBULL_DURATION:
            // Weibull distribution with param1 (scale "lambda" > 0) and param2 (shape "kappa" > 0)
            if (param1 <= 0 || param2 <=0)
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "param1(WEIBULL_DURATION:scale)", param1, "param2(WEIBULL_DURATION:shape)", param2 );
            }
            value = rng->Weibull(param1, param2);
            break;
        
        case DistributionFunction::DUAL_TIMESCALE_DURATION:
            // double exponential distribution - param1 (decay length) for the first time scale and param2 (decay length) for the second time scale.
            // param3 is percetage of time the distribution is for the first time scale
            if( param1 <= 0 )
            {
                throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "param1 (DUAL_TIMESCALE_DURATION:decay length)", param1, 0 );
            }
            if( param2 <= 0 )
            {
                throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "param2 (DUAL_TIMESCALE_DURATION:decay length)", param2, 0 );
            }
            if( (param3 < 0) || (1 < param3) )
            {
                throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "param3 (DUAL_TIMESCALE_DURATION:percentage of Time Scale #1)", param3, 0 );
            }
            if( (param3 == 1.0) || ((param3 > 0.0) && (param3 >= rng->e())) )
            {
                value = rng->expdist( param1 );
            }
            else
            {
                value = rng->expdist( param2 );
            }
            break;

        default:
            LOG_WARN_F("Do not recognize distribution type %d.  Check demographics input file.\n", distribution_flag);
            value = default_value; 
            break;

        }

        //std::cout << __FUNCTION__ << " returning value " << value << std::endl;
        return value;
    }

#define PI (3.141592653589793)

    // This method uses the Haversine Formula for calculating the great-circle distance
    // http://www.faqs.org/faqs/geography/infosystems-faq/ - Q5.1
    IDMAPI double CalculateDistanceKm( double lon_1_deg, double lat_1_deg, double lon_2_deg, double lat_2_deg )
    {
        double earth_radius_km = 6373.0f ;

        double lon_1_rad = lon_1_deg * PI/180.0;
        double lat_1_rad = lat_1_deg * PI/180.0;
        double lon_2_rad = lon_2_deg * PI/180.0;
        double lat_2_rad = lat_2_deg * PI/180.0;

        double dlon = lon_2_rad - lon_1_rad ;
        double dlat = lat_2_rad - lat_1_rad ;

        double sdlon = sin( dlon/2.0 );
        double sdlat = sin( dlat/2.0 ) ;

        double a = sdlat*sdlat + cos( lat_1_rad )*cos( lat_2_rad )*sdlon*sdlon ;
        double c = 2.0 * atan2( sqrt(a), sqrt( 1.0 - a ) );
        double d_km = c * earth_radius_km ;

        return d_km ;
    }
}
