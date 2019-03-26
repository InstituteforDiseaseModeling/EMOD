/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <cstdlib>
#include "ClimateKoppen.h"
#include "Environment.h"
#include "Common.h"
#include "Exceptions.h"


using namespace std;

#pragma warning(disable: 4244)
#pragma warning(disable: 4305)

//enum KoppenType { Af, Am, As, Aw,
//                  Bsh, Bsk, Bwh, Bwk,
//                  Cfa, Cfb, Cfc, Csa, Csb, Csc, Cwa, Cwb, Cwc,
//                  Dfa, Dfb, Dfc, Dfd, Dsa, Dsb, Dsc, Dwa, Dwb, Dwc, Dwd,
//                  EF, ET };

namespace Kernel {
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Climate.Koppen,ClimateKoppen)
    BEGIN_QUERY_INTERFACE_BODY(ClimateKoppen)
    END_QUERY_INTERFACE_BODY(ClimateKoppen)

    bool ClimateKoppen::rainfall_flip[] = { false, true, true, true,
                                                        false, false, false, false,
                                                        false, false, false, true, true, true, true, true, true,
                                                        false, false, false, false, true, true, true, true, true, true, true,
                                                        false, false };

    int ClimateKoppen::rainfall_ranges[][4] = { { 365 }, { 75, 168, 257, 351 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 },
                                                            { 365 }, { 365 }, { 365 }, { 365 },
                                                            { 365 }, { 365 }, { 365 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 },
                                                            { 365 }, { 365 }, { 365 }, { 365 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 },
                                                            { 365 }, { 365 } };

    float ClimateKoppen::rainfall_base[][4] = { { 0.2f }, { 0.03f, 0.03f, 0.4f, 0.03f }, { 0.25f, 0.02f, 0.02f, 0.02f }, { 0.02f, 0.02f, 0.25f, 0.02f },
                                                            { 0.06f }, { 0.06f }, { 0.02f }, { 0.02f },
                                                            { 0.25f }, { 0.25f }, { 0.25f }, { 0.25f, 0.06f, 0.02f, 0.06f }, { 0.25f, 0.06f, 0.02f, 0.06f }, { 0.25f, 0.06f, 0.02f, 0.06f }, { 0.02f, 0.06f, 0.25f, 0.06f }, { 0.02f, 0.06f, 0.25f, 0.06f }, { 0.02f, 0.06f, 0.25f, 0.06f },
                                                            { 0.08f }, { 0.08f }, { 0.08f }, { 0.08f }, { 0.15f, 0.06f, 0.01f, 0.06f }, { 0.15f, 0.06f, 0.01f, 0.06f }, { 0.15f, 0.06f, 0.01f, 0.06f }, { 0.01f, 0.06f, 0.15f, 0.06f }, { 0.01f, 0.06f, 0.15f, 0.06f }, { 0.01f, 0.06f, 0.15f, 0.06f }, { 0.01f, 0.06f, 0.15f, 0.06f },
                                                            { 0.02f }, { 0.05f } };

    bool ClimateKoppen::humidity_flip[] = { false, false, true, true,
                                                        false, false, false, false,
                                                        false, false, false, true, true, true, true, true, true,
                                                        false, false, false, false, true, true, true, true, true, true, true,
                                                        false, false };

    int ClimateKoppen::humidity_ranges[][4] = { { 365 }, { 365 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 },
                                                            { 365 }, { 365 }, { 365 }, { 365 },
                                                            { 365 }, { 365 }, { 365 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 },
                                                            { 365 }, { 365 }, { 365 }, { 365 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 }, { 90, 121, 274, 306 },
                                                            { 365 }, { 365 } };

    float ClimateKoppen::humidity_base[][4] = { { .75f }, { .75f }, { .7f, .55f, .45f, .55f }, { .45f, .55f, .7f, .55f },
                                                            { .45f }, { .45f }, { .3f }, { .3f },
                                                            { .75f }, { .75f }, { .75f }, { .7f, .55f, .45f, .55f }, { .7f, .55f, .45f, .55f }, { .7f, .55f, .45f, .55f }, { .45f, .55f, .7f, .55f }, { .45f, .55f, .7f, .55f }, { .45f, .55f, .7f, .55f },
                                                            { .6f }, { .6f }, { .6f }, { .6f }, { .7f, .55f, .45f, .55f }, { .7f, .55f, .45f, .55f }, { .7f, .55f, .45f, .55f }, { .45f, .55f, .7f, .55f }, { .45f, .55f, .7f, .55f }, { .45f, .55f, .7f, .55f }, { .45f, .55f, .7f, .55f },
                                                            { .6f }, { .6f } };

    float ClimateKoppen::humidity_variance[][4] = { { 0.15f }, { 0.15f }, { 0.1f, 0.05f, 0.05f, 0.05f }, { 0.05f, 0.05f, 0.1f, 0.05f },
                                                                { 0.05f }, { 0.05f }, { 0.1f }, { 0.1f },
                                                                { 0.15f }, { 0.15f }, { 0.15f }, { 0.1f, 0.05f, 0.05f, 0.05f }, { 0.1f, 0.05f, 0.05f, 0.05f }, { 0.1f, 0.05f, 0.05f, 0.05f }, { 0.05f, 0.05f, 0.1f, 0.05f }, { 0.05f, 0.05f, 0.1f, 0.05f }, { 0.05f, 0.05f, 0.1f, 0.05f },
                                                                { 0.1f }, { 0.1f }, { 0.1f }, { 0.1f }, { 0.1f, 0.05f, 0.05f, 0.05f }, { 0.1f, 0.05f, 0.05f, 0.05f }, { 0.1f, 0.05f, 0.05f, 0.05f }, { 0.05f, 0.05f, 0.1f, 0.05f }, { 0.05f, 0.05f, 0.1f, 0.05f }, { 0.05f, 0.05f, 0.1f, 0.05f }, { 0.05f, 0.05f, 0.1f, 0.05f },
                                                                { 0.1f }, { 0.1f } };

    const float ClimateKoppen::temperature_variance = 4.0;
    const float ClimateKoppen::lapse_rate = 5.0 / 1000; // decreases by 5'C/km of altitude

    ClimateKoppen * ClimateKoppen::CreateClimate( ClimateUpdateResolution::Enum update_resolution,
                                                  INodeContext * _parent,
                                                  int climate_type,
                                                  float altitude,
                                                  float latitude,
                                                  float start_time,
                                                  RANDOMBASE* pRNG )
    {
        ClimateKoppen * new_climate = _new_ ClimateKoppen(update_resolution, _parent, climate_type, altitude, latitude);
        new_climate->Configure( EnvPtr->Config );

        // initialize climate values
        new_climate->UpdateWeather( start_time, 1.0f, pRNG );

        return new_climate;
    }

    ClimateKoppen::ClimateKoppen(ClimateUpdateResolution::Enum update_resolution, INodeContext * _parent, int climate_type, float altitude, float latitude) : Climate(ClimateUpdateResolution::CLIMATE_UPDATE_MONTH, _parent)
    {
        int climate_index = climate_type - 1; // converting to 0-based type-numbers because they correspond better to array indices

        if(climate_index < Af)
        {
            //std::cerr << "Detected invalid Koppen climate-type: " << climate_type << std::endl;
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "climate_index", climate_index, Af );
        }
        else if(climate_index > ET)
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "climate_index", climate_index, ET );
        }

        koppen_type = (KoppenType)climate_index;

        in_southern_hemisphere = false;
        if(latitude <= 0)
            in_southern_hemisphere = true;

        // Calculate temperature average and range

        t_average = 20;
        t_range = 0.4 * abs(latitude);

        // See Koppen document (v3, March 3rd 2011 - Guillaume Chabot-Couture)
        // Go to http://www-das.uwyo.edu/~geerts/cwx/notes/chap16/geo_clim.html for a description of the temperature formula below.

        if(latitude >= 16)
            t_average = 44.2 - altitude*lapse_rate - abs(latitude*0.86);
        else if(latitude < 16 && latitude > -20)
            t_average = 27 - altitude*lapse_rate;
        else if(latitude < -20)
            t_average = 37.08 - altitude*lapse_rate - abs(latitude*0.63);

        // set the RoomUp and RoomDown variables using koppen_type, etc.
        float RoomUp = 0, RoomDown = 0;
        RoomUpDown(t_average, t_range, RoomUp, RoomDown);

        if (RoomUp < 0 || RoomDown < 0)
        {
            //need to shift and/or scale range
            float condition_tol = .001;
            if (RoomUp + RoomDown < -1 * condition_tol)
            {
                //implements a variant of mid-point search to find the 
                //minimum change in Tr necessary to be able to simply shift t_average

                RoomUpDown(t_average, 0, RoomUp, RoomDown);

                float bad_Tr = t_range;
                float good_Tr = 60;

                if (RoomUp + RoomDown > 0)
                    good_Tr = 0;

                float Tr_chg = abs(good_Tr - t_range);
                while (Tr_chg > condition_tol)
                {
                    t_range = (good_Tr + bad_Tr) / 2;

                    RoomUpDown(t_average, t_range, RoomUp, RoomDown);

                    if (RoomUp + RoomDown > 0)
                    {
                        //if the middle is good, shift good_Tr
                        Tr_chg = abs(good_Tr - t_range);
                        good_Tr = t_range;
                    }
                    else
                    {
                        //if the middle is bad, shift bad_Tr
                        Tr_chg = abs(bad_Tr - t_range);
                        bad_Tr = t_range;
                    }
                }
            }

            //shift, you've picked the right t_range, if you had to
            if (RoomUp < 0) //shift down
                t_average += RoomUp;
            else if (RoomDown < 0) //shift up
                t_average -= RoomDown;
        }
    }

    bool
    ClimateKoppen::Configure(
        const Configuration* config
    )
    {
        return Climate::Configure( config );
    }

    bool ClimateKoppen::IsPlausible()
    {
        if( t_average + (t_range / 2) + (2 * temperature_variance) > max_airtemp ||
            t_average - (t_range / 2) - (2 * temperature_variance) < min_airtemp ||
            t_average + (t_range / 2) + (2 * temperature_variance) > max_landtemp ||
            t_average - (t_range / 2) - (2 * temperature_variance) < min_landtemp )
        {
            return false;
        }

        return true;
    }

    void ClimateKoppen::UpdateWeather( float time, float dt, RANDOMBASE* pRNG )
    {
        // NOTE: this assumes we always start on Jan1... if that ever changes, we'll need to fix this
        int doy = (int(time) % DAYSPERYEAR) + 1; // day-of-year, one-indexed

        m_airtemperature = m_landtemperature = t_average + (t_range/2 * sin(2*PI*(doy-1)/DAYSPERYEAR + (in_southern_hemisphere ? PI/2 : 3*PI/2)));

        // calculate the actual rainfall value
        int rain_index = 0;
            int num_rain_ranges = sizeof(rainfall_ranges[koppen_type]) / sizeof(int);

            while(doy > rainfall_ranges[koppen_type][rain_index] && ++rain_index < num_rain_ranges) ;

            if(rain_index == num_rain_ranges) // must be after last separator, in range that wraps around to the start of the year
                rain_index = 0;

            if(in_southern_hemisphere && rainfall_flip[koppen_type])
                rain_index = (rain_index + (num_rain_ranges / 2)) % num_rain_ranges;
        m_accumulated_rainfall = rainfall_base[koppen_type][rain_index] * resolution_correction * dt;

        // calculate the actual humidity value
        humidity_index = 0; // we keep track of this in a class-member, since we'll need it later when adding stochasaticity
            int num_humidity_ranges = sizeof(humidity_ranges[koppen_type]) / sizeof(int);

            while(doy > humidity_ranges[koppen_type][humidity_index] && ++humidity_index < num_humidity_ranges) ;

            if(humidity_index == num_humidity_ranges) // must be after last separator, in range that wraps around to the start of the year
                humidity_index = 0;

            if(in_southern_hemisphere && humidity_flip[koppen_type])
                humidity_index = (humidity_index + (num_humidity_ranges / 2)) % num_humidity_ranges;
        m_humidity = humidity_base[koppen_type][humidity_index];

        Climate::UpdateWeather( time, dt, pRNG ); // call base-class UpdateWeather() to add stochasticity and check values are within valid bounds
    }

    void ClimateKoppen::AddStochasticity( RANDOMBASE* pRNG, float, float, bool, float)
    {
        Climate::AddStochasticity( pRNG, temperature_variance, temperature_variance, true, humidity_variance[koppen_type][humidity_index] );
    }

    void ClimateKoppen::RoomUpDown(const float Ta, const float Tr, float &RoomUp, float &RoomDown)
    {
        switch((int)koppen_type) // cast necessary to avoid gcc ambiguity error
        {
        case Af:
        case Am:
        case As:
        case Aw:
            RoomUp = 1000; // effectively infinity
            RoomDown = Ta- Tr/2 - 18;
            break;

        case Bsh:
        case Bwh:
            RoomUp = 1000; // effectively infinity
            RoomDown = Ta - 18;
            break;

        case Bsk:
        case Bwk:
            RoomUp = 18 - Ta;
            RoomDown = 1000;  // effectively infinity
            break;

        case Cfa:
        case Csa:
        case Cwa:
            RoomUp = 1000; // effectively infinity
            RoomDown = min((float)(Ta + Tr/2 - 22), (float)(Ta - Tr/2 + 3));
            break;

        case Cfb:
        case Csb:
        case Cwb:
            RoomUp = 22 - (Ta + Tr/2);
            RoomDown = min((float)(Ta + Tr/2 * sin(PI/6) - 10), (float)(Ta - Tr/2 + 3));
            break;

        case Cfc:
        case Csc:
        case Cwc:
            RoomUp = 10 - (Ta + Tr/2 * sin(PI/4));
            RoomDown = Ta - Tr/2 + 3;
            break;

        case Dfa:
        case Dsa:
        case Dwa:
            RoomUp = -3 - (Ta - Tr/2);
            RoomDown = Ta + Tr/2 - 22;
            break;

        case Dfb:
        case Dsb:
        case Dwb:
            RoomUp = min(22 - (Ta + Tr/2), -3 - (Ta - Tr/2));
            RoomDown = Ta + Tr/2 * sin(PI/6) - 10;
            break;

        case Dfc:
        case Dsc:
        case Dwc:
            RoomUp = min((float)(10 - (Ta + Tr/2 * sin(PI/4))),(float)( -3 - (Ta - Tr/2)));
            RoomDown = Ta + Tr/2 - 10;
            break;

        case Dfd:
        case Dwd:
            RoomUp = min((float)(10 - (Ta + Tr/2 * sin(PI/4))), (float)(-38 - (Ta - Tr/2)));
            RoomDown = Ta + Tr/2 - 10;
            break;

        case EF:
            RoomUp = 0 - (Ta + Tr/2);
            RoomDown = 1000;  // effectively infinity
            break;

        case ET:
            RoomUp = 10 - (Ta + Tr/2);
            RoomDown = 1000;  // effectively infinity
            break;
        }
    }
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, ClimateKoppen & climate, const unsigned int file_version)
    {
        ar & climate.koppen_type;
        ar & climate.t_average;
        ar & climate.t_range;
        ar & climate.in_southern_hemisphere;
        ar & boost::serialization::base_object<Kernel::Climate>(climate);
    }
}
#endif
