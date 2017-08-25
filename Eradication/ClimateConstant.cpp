/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <cstdlib>
#include "ClimateConstant.h"
#include "Contexts.h"
#include "Common.h"
#include "Debug.h"

#include "SimulationConfig.h"

SETUP_LOGGING( "ClimateConstant" )

namespace Kernel {
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Climate.Constant,ClimateConstant)
    BEGIN_QUERY_INTERFACE_BODY(ClimateConstant)
    END_QUERY_INTERFACE_BODY(ClimateConstant)

    ClimateConstant * ClimateConstant::CreateClimate(ClimateUpdateResolution::Enum update_resolution, INodeContext * _parent, float start_time)
    {
        ClimateConstant * new_climate = _new_ ClimateConstant(update_resolution, _parent);
        release_assert( new_climate );
        new_climate->Configure( EnvPtr->Config );

        // initialize climate values
        new_climate->UpdateWeather(start_time, 1.0f);

        return new_climate;
    }

    ClimateConstant::ClimateConstant()
    { }
    ClimateConstant::ClimateConstant(ClimateUpdateResolution::Enum update_resolution, INodeContext * _parent)
    : Climate(update_resolution, _parent)
    {
    }

    bool
    ClimateConstant::Configure(
        const Configuration* config
    )
    {
        LOG_DEBUG( "Configure\n" );

        initConfigTypeMap( "Base_Air_Temperature", &base_airtemperature, Base_Air_Temperature_DESC_TEXT, -55.0f, 45.0f, 22.0f );
        initConfigTypeMap( "Base_Land_Temperature", &base_landtemperature, Base_Land_Temperature_DESC_TEXT, -55.0f, 60.0f, 26.0f );
        initConfigTypeMap( "Base_Rainfall", &base_rainfall, Base_Rainfall_DESC_TEXT, 0.0f, 150.0f, 10.0f );
        initConfigTypeMap( "Base_Relative_Humidity", &base_humidity, Base_Relative_Humidity_DESC_TEXT, 0.0f, 1.0f, 0.75f );
        return Climate::Configure( config );
    }

    bool ClimateConstant::IsPlausible()
    {
        if( base_airtemperature + (2 * airtemperature_variance) > max_airtemp ||
            base_airtemperature - (2 * airtemperature_variance) < min_airtemp ||
            base_landtemperature + (2 * landtemperature_variance) > max_landtemp ||
            base_landtemperature - (2 * landtemperature_variance) < min_landtemp ||
            base_rainfall < 0.0 ||
            base_humidity > 1.0 ||
            base_humidity < 0.0 )
        {
            LOG_DEBUG( "IsPlausible returning false\n" );
            return false;
        }

        if((rainfall_variance_enabled && (EXPCDF(-1 /base_rainfall * max_rainfall) < 0.975)) ||
            (!rainfall_variance_enabled && base_rainfall > max_rainfall))
        {
            LOG_DEBUG( "IsPlausible returning false\n" );
            return false;
        }

        return true;
    }

    void ClimateConstant::UpdateWeather(float time, float dt)
    {
        m_airtemperature = (float)base_airtemperature;
        m_landtemperature = (float)base_landtemperature;
        m_accumulated_rainfall = (float)base_rainfall * dt;
        m_humidity = (float)base_humidity;

        Climate::UpdateWeather(time, dt); // call base-class UpdateWeather() to add stochasticity and check values are within valid bounds
    }
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, ClimateConstant& climate, const unsigned int file_version)
    {
        ar & boost::serialization::base_object<Kernel::Climate>(climate);
    }
}
#endif
