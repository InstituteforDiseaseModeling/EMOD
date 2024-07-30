
#include "stdafx.h"
#include "InterpolatedValueMap.h"

#include "Log.h"

SETUP_LOGGING( "InterpolatedValueMap" )

static const char* TIMES = "Times" ;
static const char* VALUES = "Values" ;

namespace Kernel
{
    InterpolatedValueMap::InterpolatedValueMap( float min_time, 
                                                float max_time,
                                                float min_value,
                                                float max_value )
        : m_TimeValueMap()
        , m_MinTime( min_time )
        , m_MaxTime( max_time )
        , m_MinValue( min_value )
        , m_MaxValue( max_value )
    {
    }

    bool InterpolatedValueMap::Configure( const Configuration * inputJson )
    {
        std::vector<float> times, values;
        initConfigTypeMap( TIMES,  &times,  Interpolated_Value_Map_Times_DESC_TEXT, m_MinTime,  m_MaxTime, true );
        initConfigTypeMap( VALUES, &values, Interpolated_Value_Map_Value_DESC_TEXT, m_MinValue, m_MaxValue );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            if( times.size() != values.size() )
            {
                std::stringstream ss;
                ss << "The number of elements in '" << TIMES << "' (=" << times.size() << ") "
                   << "does not match the number of elements in '" << VALUES << "' (=" << values.size() << ")." ;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            for( int i = 0; i < times.size(); ++i )
            {
                m_TimeValueMap.insert( std::make_pair( times[ i ], values[ i ] ) );
            }
        }
        return ret;
    }

    size_t InterpolatedValueMap::size() const
    {
        return m_TimeValueMap.size();
    }

    std::map<float, float>::const_iterator InterpolatedValueMap::begin() const
    {
        return m_TimeValueMap.begin();
    }

    std::map<float, float>::const_iterator InterpolatedValueMap::end() const
    {
        return m_TimeValueMap.end();
    }

    std::map<float, float>::const_reverse_iterator InterpolatedValueMap::rbegin() const
    {
        return m_TimeValueMap.rbegin();
    }

    std::map<float, float>::const_reverse_iterator InterpolatedValueMap::rend() const
    {
        return m_TimeValueMap.rend();
    }

    void InterpolatedValueMap::add( float time, float value )
    {
        m_TimeValueMap.insert( std::make_pair( time, value ) );
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
        for( auto &entry: m_TimeValueMap )
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
            ret_rdd = m_TimeValueMap.at( float(map_year) );
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
        if (m_TimeValueMap.empty())
        {
            return default_value;
        }

        // if the first year in the map is greater than the current year, return the default value
        previous_year = m_TimeValueMap.begin()->first;
        if (previous_year > year)
        {
            return default_value;
        }

        // go through the map entries and do linear interpolation on the value
        previous_value = m_TimeValueMap.at(previous_year);
        for (auto &entry : m_TimeValueMap)
        {
            map_year = entry.first;
            map_value = m_TimeValueMap.at(map_year);

            // if the map_year is greater than the input year, return the liner interpolation of the value
            if (map_year > year)
            {
                if (map_year == previous_year)
                {
                    return m_TimeValueMap.at(map_year);
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
        auto it = m_TimeValueMap.rbegin();
        float last_year = it->first;
        bool at_end = (last_year <= currentYear);
        return at_end;
    }

    void InterpolatedValueMap::serialize( IArchive& ar, InterpolatedValueMap& mapping )
    {
        size_t count = ar.IsWriter() ? mapping.m_TimeValueMap.size() : -1;

        ar.startArray(count);
        if( ar.IsWriter() )
        {
            for( auto& entry : mapping.m_TimeValueMap )
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
                mapping.m_TimeValueMap[key] = value;
            }
        }
        ar.endArray();
    }
}
