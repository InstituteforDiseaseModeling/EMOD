/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#define DAYSPERYEAR         (365)
#define IDEALDAYSPERMONTH   (30)
#define DAYSPERWEEK         (7)
#define HOURSPERDAY         (24)
#define MONTHSPERYEAR       (12)

#define ARCMINUTES_PER_DEGREE   (60)
#define KM_PER_ARCMINUTE        (1.86)
#define EARTH_RADIUS_KM         (6371.2213)

#define EXPCDF(x)   (1 - exp(x))

#define NO_LESS_THAN( x, y ) { if ( x < y ) { x = y; } }
#define NO_MORE_THAN( x, y ) { if ( x > y ) { x = y; } }

struct AffectedPopulation {
    enum _enum {
        Minimum                 = 1,
        Everyone                = 1,
        ChildrenUnderFive       = 2,
        PossibleMothers         = 3,
        Infants                 = 4,    // <18 months
        ArrivingAirTravellers   = 5,
        DepartingAirTravellers  = 6,
        ArrivingRoadTravellers  = 7,
        DepartingRoadTravellers = 8,
        AllArrivingTravellers   = 9,
        AllDepartingTravellers  = 10,
        Maximum                 = 10
    };
};

struct InfectionStateChange {
    enum _enum {
        None            = 0,
        Cleared         = 1,
        Fatal           = 2,
        New             = 3,

        // polio only
        PolioParalysis_WPV1  = 4,
        PolioParalysis_WPV2  = 5,
        PolioParalysis_WPV3  = 6,
        PolioParalysis_VDPV1 = 7,
        PolioParalysis_VDPV2 = 8,
        PolioParalysis_VDPV3 = 9,

        // tb only
        TBLatent        = 10,
        TBActivation    = 11,
        TBInactivation  = 12,
        TBActivationSmearPos = 13,
        TBActivationSmearNeg = 14,
        TBActivationExtrapulm = 15,
        ClearedPendingRelapse = 16,
        TBActivationPresymptomatic = 17
    };
};

enum struct HumanStateChange : unsigned int {
    None                  = 0,
    DiedFromNaturalCauses = 1,
    KilledByInfection     = 2,
    KilledByCoinfection   = 3,
    Migrating             = 10
};

struct NewInfectionState {
    enum _enum {
        Invalid        = 0,
        NewInfection   = 1,
        NewlyDetected  = 2,
        NewAndDetected = 3,

        // tb only
        NewlyActive    = 7,
        NewlyInactive  = 8,
        NewlyCleared   = 9
    };
};

#define MAX_LOCAL_MIGRATION_DESTINATIONS    (8)
#define MAX_AIR_MIGRATION_DESTINATIONS      (60)
#define MAX_REGIONAL_MIGRATION_DESTINATIONS (30)
#define MAX_SEA_MIGRATION_DESTINATIONS      (5)

#define MAXIMUM_TRAVEL_WAYPOINTS    (10)

#define INFINITE_TIME   (365*1000)  // a full millennium
#define MAX_HUMAN_LIFETIME   45000  // ~123 years
#define MAX_HUMAN_AGE       (125.0f)

#define MAX_INTERVENTION_TYPE   (20)

#define DEFAULT_BIRTHRATE           (0.25f)
#define DEFAULT_POVERTY_THRESHOLD   (0.5f)

#define DEFAULT_VACCINE_COST    (10)

struct SpatialCoverageType {
    enum _enum {
        None          = 0,
        All           = 1,
        CommunityList = 2,
        Polygon       = 4
    };
};

#define WEEKS_FOR_GESTATION   (40)

#define BIRTHRATE_SANITY_VALUE  (0.005) // 1 birth per person per 200 days
#define TWO_PERCENT_PER_YEAR    (0.02 / DAYSPERYEAR)
#define INDIVIDUAL_BIRTHRATE    (1.0 / 8 / DAYSPERYEAR) // 1 child every 8 years of fertility (about 4 total)
#define FALLBACK_BIRTHRATE      (0.001)

#define CHECK_INDEX(_i, _min, _count) assert((_i >= _min) && (_i < _count))

#define MILLIMETERS_PER_METER   (1000.0f)

////////////////////////////////////////

#ifndef ZERO_ITEM
#define ZERO_ITEM(_item)    memset(&(_item), 0, sizeof(_item))
#endif

#ifndef ZERO_ARRAY
#define ZERO_ARRAY(_array)  memset(_array, 0, sizeof(_array))
#endif

#ifndef INIT_ITEM
#define INIT_ITEM(_item) assert(sizeof(_item) == sizeof(init_##_item)); memcpy(&_item, init_##_item, sizeof(_item))
#endif

#ifndef INIT_ARRAY
#define INIT_ARRAY(_array) assert(sizeof(_array) == sizeof(init_##_array)); memcpy(_array, init_##_array, sizeof(_array))
#endif

#include "Environment.h"

#ifdef VALIDATION
#define LOG(_val)    EnvPtr->Report.Validation->Log(boost::format("%1% <= %2%") % _name % (_t)(_val))
#else
#define LOG(_val)
#endif

template<class _t>
class Property
{
protected:
    _t data;
#ifdef VALIDATION
    char *_name;
    Property<_t>() {}
    Property<_t>(_t init) : data(init) {}
#endif

public:
    operator _t() { return data; }
    template <class _v>
    _t& operator =(_v value)
    {
        LOG(value);
        data = value;
        return data;
    }
    template <class _v>
    _t& operator +=(_v value)
    {
        LOG(_t(data + value));
        data += value;
        return data;
    }
    template <class _v>
    _t& operator -=(_v value)
    {
        LOG(_t(data - value));
        data -= value;
        return data;
    }
    template <class _v>
    _t& operator *=(_v value)
    {
        LOG(_t(data * value));
        data *= value;
        return data;
    }
    // prefix operator
    _t& operator ++()
    {
        ++data;
        LOG(data);
        return data;
    }
    // postfix operator
    _t& operator ++(int)
    {
        _t temp = data++;
        LOG(data);
        return temp;
    }
    // prefix operator
    _t& operator --()
    {
        --data;
        LOG(data);
        return data;
    }
    // postfix operator
    _t& operator --(int)
    {
        _t temp = data--;
        LOG(data);
        return temp;
    }

#if USE_BOOST_SERIALIZATION
        ///////////////////////////////////////////////////////////////////////////
        // Serialization
        friend class ::boost::serialization::access;

        template<class Archive>
        void serialize(Archive &ar, const unsigned int v)
        {
            ar & data;
        };
        ///////////////////////////////////////////////////////////////////////////
#endif
};

#ifdef VALIDATION

#define PROPERTY(_t, _v) \
class _##_v \
{ \
protected: \
    _t data; \
    char *name; \
    boost::format fmt; \
public: \
    _##_v() : fmt("%1% <= %2%") { name = #_v; } \
    _##_v(_t init) : fmt("%1% <= %2%") \
    { \
        name = #_v; \
        EnvPtr->Report.Validation->Log(boost::format("%1% <= %2%") % name % init); \
        data = init; \
    } \
public: \
    operator _t () { return data; } \
    template <class V> \
    _t & operator =(const V value) \
    { \
        EnvPtr->Report.Validation->Log(boost::format("%1% <= %2%") % name % (_t)(value)); \
        data = value; \
        return data; \
    } \
    template <class V> \
    _t & operator +=(V value) \
    { \
        EnvPtr->Report.Validation->Log(boost::format("%1% <= %2%") % name % (_t)(data + value)); \
        data += value; \
        return data; \
    } \
    template <class V> \
    _t & operator -=(V value) \
    { \
        EnvPtr->Report.Validation->Log(boost::format("%1% <= %2%") % name % (_t)(data - value)); \
        data -= value; \
        return data; \
    } \
    template <class V> \
    _t & operator *=(V value) \
    { \
        EnvPtr->Report.Validation->Log(boost::format("%1% <= %2%") % name % (_t)(data * value)); \
        data *= value; \
        return data; \
    } \
    _t & operator ++() \
    { \
        ++data; \
        EnvPtr->Report.Validation->Log(boost::format("%1% <= %2%") % name % data); \
        return data; \
    } \
    _t operator ++(int) \
    { \
        _t temp = data++; \
        EnvPtr->Report.Validation->Log(boost::format("%1% <= %2%") % name % data); \
        return temp; \
    } \
    _t & operator --() \
    { \
        --data; \
        EnvPtr->Report.Validation->Log(boost::format("%1% <= %2%") % name % data); \
        return data; \
    } \
    _t operator --(int) \
    { \
        _t temp = data--; \
        EnvPtr->Report.Validation->Log(boost::format("%1% <= %2%") % name % data); \
        return temp; \
    } \
    template <class Archive> \
    void serialize(Archive &ar, const unsigned int version) { ar & data; } \
} _v

#else

#define PROPERTY(_t, _v) _t _v

#endif
