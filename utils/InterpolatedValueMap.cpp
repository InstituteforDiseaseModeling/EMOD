/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "InterpolatedValueMap.h"

#include "Log.h"

SETUP_LOGGING( "InterpolatedValueMap" )

static const std::string TIMES = "Times" ;
static const std::string VALUES = "Values" ;

namespace Kernel
{
    template< typename T >
    void checkRange( const char* parameterName, T minValue, T value, T maxValue )
    {
        if( value < minValue )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__,
                                               parameterName, value, minValue );
        }
        else if( value > maxValue )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__,
                                               parameterName, value, maxValue );
        }
    }

    InterpolatedValueMap::InterpolatedValueMap( float min_time, 
                                                float max_time,
                                                float min_value,
                                                float max_value )
        : tFloatFloatMapConfigType()
        , m_MinTime( min_time )
        , m_MaxTime( max_time )
        , m_MinValue( min_value )
        , m_MaxValue( max_value )
    {
    }

    void
    InterpolatedValueMap::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        std::vector< float > times, values;

        // Temporary object created so we can 'operate' on json with the desired tools
        auto config = Configuration::CopyFromElement( (*inputJson)[key], inputJson->GetDataLocation() );
        // Do not copy this pattern. This 'low-level' API to parse out values is not preferred.
        times = GET_CONFIG_VECTOR_FLOAT( config, TIMES );
        values = GET_CONFIG_VECTOR_FLOAT( config, VALUES );

        if( times.size() != values.size() )
        {
            std::stringstream ss;
            ss << key << ": The number of elements in " << TIMES << " (=" << times.size() << ") "
               << "does not match the number of elements in " << VALUES << " (=" << values.size() << ")." ;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        std::string time_str  = key + ":" + TIMES ;
        std::string value_str = key + ":" + VALUES ;

        float prev_time = m_MinTime ;

        // Now we have the values in our local variables, populate our map.
        for( auto idx=0; idx<times.size(); idx++ )
        {
            checkRange<float>( time_str.c_str(),  m_MinTime,  times[idx],  m_MaxTime  );
            checkRange<float>( value_str.c_str(), m_MinValue, values[idx], m_MaxValue );

            if( (idx > 0) && (times[idx] <= prev_time) )
            {
                std::stringstream ss;
                ss << key << ":" << TIMES << " - Element number " << (idx+1) << " (=" << times[idx] << ") is <= element number "
                   << (idx) << " (=" << times[idx-1] << ").  '" << TIMES << "' must be in increasing order." ;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            (this)->insert( std::make_pair( times[idx], values[idx] ) );
            LOG_DEBUG_F( "Inserting year %f and delay %f into map.\n", times[idx], values[idx] );

            prev_time = times[idx] ;
        }

        delete config;
        config = nullptr;
    }

    json::QuickBuilder
    InterpolatedValueMap::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:InterpolatedValueMap" );
    
        schema[ts] = json::Object();
        schema[ts][TIMES] = json::Array();
        schema[ts][TIMES][0][ "type" ] = json::String( "float" );
        schema[ts][TIMES][0][ "min" ] = json::Number( m_MinTime );
        schema[ts][TIMES][0][ "max" ] = json::Number( m_MaxTime );
        schema[ts][TIMES][0][ "description" ] = json::String( Interpolated_Value_Map_Times_DESC_TEXT );
        schema[ts][VALUES] = json::Array();
        schema[ts][VALUES][0][ "type" ] = json::String( "float" );
        schema[ts][VALUES][0][ "min" ] = json::Number( m_MinValue );
        schema[ts][VALUES][0][ "max" ] = json::Number( m_MaxValue );
        schema[ts][VALUES][0][ "description" ] = json::String( Interpolated_Value_Map_Value_DESC_TEXT );
        return schema;
    }

    float
    InterpolatedValueMap::getValuePiecewiseConstant(
        float current_year,
        float default_value
    )
    const
    {
        float ret_rdd = default_value;

        // go through years in map, 2000->2005->2014. Stop as soon as we get to year that is greater than our current year. keep previous value.
        for( auto &entry: (*this) )
        {
            auto map_year = entry.first;
            //std::cout << "map_year = " << map_year << std::endl;
            //std::cout << "current_year = " << current_year << std::endl;
            if( map_year > current_year )
            {
                //LOG_DEBUG_F( "Breaking from lookup loop with %f as delay days because map year %d is > current year %d\n", remaining_delay_days, map_year, current_year );
                break;
            }
            //ret_rdd = (int)year2DelayMap[ map_year ];
            ret_rdd = (*this).at( float(map_year) );
        }
        return ret_rdd;
    }


    float
    InterpolatedValueMap::getValueLinearInterpolation(
        float year,
        float default_value
    )
    const
    {
        float map_year, previous_year;
        float map_value, previous_value;

        // if there are no values in the map, return the default value
        if (this->empty())
        {
            return default_value;
        }

        // if the first year in the map is greater than the current year, return the default value
        previous_year = (*this->begin()).first;
        if (previous_year > year)
        {
            return default_value;
        }

        // go through the map entries and do linear interpolation on the value
        previous_value = this->at(previous_year);
        for (auto &entry : (*this))
        {
            map_year = entry.first;
            map_value = this->at(map_year);

            // if the map_year is greater than the input year, return the liner interpolation of the value
            if (map_year > year)
            {
                if (map_year == previous_year)
                {
                    return this->at(map_year);
                }
                return ((year - previous_year)/(map_year - previous_year)) * (map_value - previous_value) + previous_value;
            }

            // update the previous year and value
            previous_year = map_year;
            previous_value = map_value;
        }

        // if we made it here, the year is after the last mapped year.  return the last value.
        return map_value;
    }

    bool InterpolatedValueMap::isAtEnd( float currentYear ) const
    {
        auto it = this->rend();
        ++it;
        float last_year = it->first;
        bool at_end = (last_year <= currentYear);
        return at_end;
    }

    void InterpolatedValueMap::serialize( IArchive& ar, InterpolatedValueMap& mapping )
    {
        size_t count = ar.IsWriter() ? mapping.size() : -1;

        ar.startArray(count);
        if( ar.IsWriter() )
        {
            for( auto& entry : mapping )
            {
                float key   = entry.first;
                float value = entry.second;
                ar.startObject();
                    ar.labelElement("key"  ) & key;
                    ar.labelElement("value") & value;
                ar.endObject();
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                float key=0.0;
                float value=0.0;
                ar.startObject();
                    ar.labelElement("key"  ) & key;
                    ar.labelElement("value") & value;
                ar.endObject();
                mapping[key] = value;
            }
        }
        ar.endArray();
    }
}
