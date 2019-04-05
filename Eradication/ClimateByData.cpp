/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "ClimateByData.h"
#include "Environment.h"
#include "Common.h"
#include "Exceptions.h"

#ifdef WIN32
#include <float.h>
#else
#include <cmath>
#endif

SETUP_LOGGING( "ClimateByData" )

namespace Kernel {
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Climate.ByData,ClimateByData)
    BEGIN_QUERY_INTERFACE_BODY(ClimateByData)
    END_QUERY_INTERFACE_BODY(ClimateByData)

    ClimateByData * ClimateByData::CreateClimate( ClimateUpdateResolution::Enum update_resolution,
                                                  INodeContext * _parent,
                                                  int datapoints,
                                                  std::ifstream& airtemperature_file,
                                                  std::ifstream& landtemperature_file,
                                                  std::ifstream& rainfall_file,
                                                  std::ifstream& humidity_file,
                                                  float start_time,
                                                  RANDOMBASE* pRNG )
    {
        ClimateByData * new_climate = _new_ ClimateByData(update_resolution, _parent);
        new_climate->Configure( EnvPtr->Config );

        new_climate->ReadDataFromFiles(datapoints, airtemperature_file, landtemperature_file, rainfall_file, humidity_file);

        // initialize climate values
        new_climate->UpdateWeather( start_time, 1.0f, pRNG );

        return new_climate;
    }

    ClimateByData::ClimateByData() { }
    ClimateByData::ClimateByData(ClimateUpdateResolution::Enum update_resolution, INodeContext * _parent) : Climate(update_resolution, _parent) { }

    bool
    ClimateByData::Configure(
        const Configuration* config
    )
    {
        LOG_DEBUG( "Configure\n" );
        initConfigTypeMap( "Air_Temperature_Offset", &airtemperature_offset, Air_Temperature_Offset_DESC_TEXT, -20.0f, 20.0f, 0.0f, "Climate_Model","CLIMATE_BY_DATA" );
        initConfigTypeMap( "Land_Temperature_Offset", &landtemperature_offset, Land_Temperature_Offset_DESC_TEXT, -20.0f, 20.0f, 0.0f,"Climate_Model","CLIMATE_BY_DATA" );
        initConfigTypeMap( "Rainfall_Scale_Factor", &rainfall_scale_factor, Rainfall_Scale_Factor_DESC_TEXT, 0.1f, 10.0f, 1.0f, "Climate_Model", "CLIMATE_BY_DATA" );
        initConfigTypeMap( "Relative_Humidity_Scale_Factor", &humidity_scale_factor, Relative_Humidity_Scale_Factor_DESC_TEXT, 0.1f, 10.0f, 1.0f, "Climate_Model", "CLIMATE_BY_DATA" );
        return Climate::Configure( config );
    }

    bool ClimateByData::IsPlausible()
    {

        // check to see whether fewer than 2.5% of the values will exceed the upper- and lower-bounds

        int low_index = int(num_datapoints * 0.025);
        int high_index = int(num_datapoints * 0.975);

        std::vector<float> sorted = airtemperature_data;
        sort(sorted.begin(), sorted.end());

        if( sorted[high_index] + (2 * airtemperature_variance) > max_airtemp ||
            sorted[low_index] - (2 * airtemperature_variance) < min_airtemp )
        {
            return false;
        }

        sorted = landtemperature_data;
        sort(sorted.begin(), sorted.end());

        if( sorted[high_index] + (2 * landtemperature_variance) > max_landtemp ||
            sorted[low_index] - (2 * landtemperature_variance) < min_landtemp )
        {
            return false;
        }

        sorted = humidity_data;
        sort(sorted.begin(), sorted.end());

        if( sorted[high_index] > 1.0 || sorted[low_index] < 0.0 )
            return false;

        sorted = rainfall_data;
        sort(sorted.begin(), sorted.end());

        if(sorted[low_index] < 0)
            return false;

        if((rainfall_variance_enabled && (EXPCDF(-1 / sorted[high_index] * resolution_correction * max_rainfall) < 0.975)) ||
            (!rainfall_variance_enabled && sorted[high_index] * resolution_correction > max_rainfall))
        {
            return false;
        }

        return true;
    }

#ifdef WIN32
#define _ISNAN_(_f) _isnan(_f)
#define _ISINF_(_f) !_finite(_f)
#else
#define _ISNAN_(_f) std::isnan(_f)
#define _ISINF_(_f) std::isinf(_f)
#endif

    void CheckForNanOrInf( std::vector<float>& data, const char* source )
    {
        uint32_t index = 0;
        for (float value : data)
        {
            if ( _ISNAN_( value ) )
            {
                // Consider OutOfRangeException( const char * file_name, int line_num, const char* func_name, const char* var_name = nullptr, float value = 0.0f, float value_violated = 0.0f );
                std::ostringstream msg;
                msg << "Found NaN value in " << source << " for current node at offset " << index << '.' << std::endl;
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            if ( _ISINF_( value ) )
            {
                // Consider OutOfRangeException( const char * file_name, int line_num, const char* func_name, const char* var_name = nullptr, float value = 0.0f, float value_violated = 0.0f );
                std::ostringstream msg;
                msg << "Found infinite value in " << source << " for current node at offset " << index << '.' << std::endl;
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            ++index;
        }
    }

    void ClimateByData::ReadDataFromFiles(int datapoints, std::ifstream& airtemperature_file, std::ifstream& landtemperature_file, std::ifstream& rainfall_file, std::ifstream& humidity_file)
    {
        LOG_DEBUG( "ReadDataFromFiles\n" );
        airtemperature_data.resize(datapoints);
        landtemperature_data.resize(datapoints);
        rainfall_data.resize(datapoints);
        humidity_data.resize(datapoints);

        num_datapoints = datapoints;
        num_years = int((float(num_datapoints) / (resolution_correction * DAYSPERYEAR)) + 0.01 /* compensate for rounding errors */);

        airtemperature_file.read(  (char*)airtemperature_data.data(),  datapoints*sizeof(float) );
        landtemperature_file.read( (char*)landtemperature_data.data(), datapoints*sizeof(float) );
        rainfall_file.read(        (char*)rainfall_data.data(),        datapoints*sizeof(float) );
        humidity_file.read(        (char*)humidity_data.data(),        datapoints*sizeof(float) );

        // TODO: in the future, we may want to do this scaling in a separate function and have 
        //       some sort of bool that checks whether the array has been initialized; that way
        //       if we do multiple runs with different scale values, things work as expected

        // NaN's are very bad and, unfortunately, sometimes creep into climate files.
        CheckForNanOrInf( airtemperature_data, "air temperature data" );
        CheckForNanOrInf( landtemperature_data, "land temperature data" );
        CheckForNanOrInf( rainfall_data, "rainfall data" );
        CheckForNanOrInf( humidity_data, "relative humidity data" );

        // apply scaling factors to data


        if(airtemperature_offset != 0.0f)
            for(int i = 0; i < datapoints; i++)
                airtemperature_data[i] += airtemperature_offset;

        if(landtemperature_offset != 0.0f)
            for(int i = 0; i < datapoints; i++)
                landtemperature_data[i] += landtemperature_offset;

        // correct rainfall from mm to m
        float scale = 1.0f / MILLIMETERS_PER_METER;
        if(rainfall_scale_factor != 1.0f)
            scale *= rainfall_scale_factor;

        for(int i = 0; i < datapoints; i++)
            rainfall_data[i] *= scale;

        if(humidity_scale_factor != 1.0f)
            for(int i = 0; i < datapoints; i++)
                humidity_data[i] *= humidity_scale_factor;
    }

    void ClimateByData::UpdateWeather( float time, float dt, RANDOMBASE* pRNG )
    {
        LOG_DEBUG_F("UpdateWeather: time = %f", time);
        int index = 0;

        // Figure out index based on time and number of years of data
        float adjusted_time = time - (DAYSPERYEAR * num_years * int(time / (DAYSPERYEAR * num_years))); // adjust into n-year window

        // now adjust based on number of data points spread over n years, unless monthly, in which case use old way with specific months (28-day February, etc...)
        if(resolution_correction == 1.0f / 30) // monthly
        {
            if(int(adjusted_time)%DAYSPERYEAR < 31)       { index = 0; }
            else if(int(adjusted_time)%DAYSPERYEAR < 59)  { index = 1; }
            else if(int(adjusted_time)%DAYSPERYEAR < 90)  { index = 2; }
            else if(int(adjusted_time)%DAYSPERYEAR < 120) { index = 3; }
            else if(int(adjusted_time)%DAYSPERYEAR < 151) { index = 4; }
            else if(int(adjusted_time)%DAYSPERYEAR < 181) { index = 5; }
            else if(int(adjusted_time)%DAYSPERYEAR < 212) { index = 6; }
            else if(int(adjusted_time)%DAYSPERYEAR < 243) { index = 7; }
            else if(int(adjusted_time)%DAYSPERYEAR < 273) { index = 8; }
            else if(int(adjusted_time)%DAYSPERYEAR < 304) { index = 9; }
            else if(int(adjusted_time)%DAYSPERYEAR < 334) { index = 10; }
            else if(int(adjusted_time)%DAYSPERYEAR < DAYSPERYEAR) { index = 11; }
            index += int(adjusted_time / DAYSPERYEAR) * 12;
        }
        else // everything except monthly
        {
            index = int(adjusted_time * resolution_correction);
        }

        // verify the index is not out of range
        if(index >= num_datapoints)
        {
            //std::cerr << "Climate-By-Data index (" << index << ") is outside data-array range. (num_datapoints = " << num_datapoints << ")." << std::endl;
            throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "index", index, num_datapoints );
        }

        //Now set data-values
        m_airtemperature = airtemperature_data[index];
        m_landtemperature = landtemperature_data[index];
        m_accumulated_rainfall = rainfall_data[index] * resolution_correction * dt; // just the mean rainfall each day
        m_humidity = humidity_data[index];

        Climate::UpdateWeather( time, dt, pRNG ); // call base-class UpdateWeather() to add stochasticity and check values are within valid bounds
    }

    ClimateByData::~ClimateByData()
    {

    }
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, ClimateByData &climate, const unsigned int v)
    {
        ar & climate.num_datapoints;
        ar & climate.num_years
           & climate.airtemperature_data
           & climate.landtemperature_data
           & climate.rainfall_data
           & climate.humidity_data;

        ar & boost::serialization::base_object<Kernel::Climate>(climate);
    }
}
#endif
