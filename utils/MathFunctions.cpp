
#include <stdafx.h>

#include "MathFunctions.h"
#include "Environment.h"
#include "RANDOM.h"
#include "Log.h"
#include "Debug.h"


SETUP_LOGGING("MathFunctions")

namespace Kernel
{
#define PI (3.141592653589793)

    // This method uses the Haversine Formula for calculating the great-circle distance
    // http://www.faqs.org/faqs/geography/infosystems-faq/ - Q5.1
    IDMAPI double CalculateDistanceKm(double lon_1_deg, double lat_1_deg, double lon_2_deg, double lat_2_deg)
    {
        double earth_radius_km = 6373.0f;

        double lon_1_rad = lon_1_deg * PI / 180.0;
        double lat_1_rad = lat_1_deg * PI / 180.0;
        double lon_2_rad = lon_2_deg * PI / 180.0;
        double lat_2_rad = lat_2_deg * PI / 180.0;

        double dlon = lon_2_rad - lon_1_rad;
        double dlat = lat_2_rad - lat_1_rad;

        double sdlon = sin(dlon / 2.0);
        double sdlat = sin(dlat / 2.0);

        double a = sdlat*sdlat + cos(lat_1_rad)*cos(lat_2_rad)*sdlon*sdlon;
        double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
        double d_km = c * earth_radius_km;

        return d_km;
    }
}

float Kernel::NTimeStepProbability(NonNegativeFloat PerTimeStepProbability, float dt)
{
    if (PerTimeStepProbability > 1.0)
    {
        return 1.0;
    }

    if (dt <= 1.0)
    {
        return PerTimeStepProbability *  dt;
    }
    else
    {
        return (1.0 - pow(1.0 - PerTimeStepProbability, dt));
    }
}
