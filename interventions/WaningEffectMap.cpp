/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "WaningEffectMap.h"
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"
#include "IArchive.h"

static const char* _module = "WaningEffectMapAbstract";

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- WaningEffectMapLinear
    // ------------------------------------------------------------------------
    IMPL_QUERY_INTERFACE2(WaningEffectMapAbstract, IWaningEffect, IConfigurable)

    WaningEffectMapAbstract::WaningEffectMapAbstract()
    : IWaningEffect()
    , m_Expired(false)
    , m_EffectOriginal(0.0)
    , m_EffectCurrent(0.0)
    , m_ExpireAtDurationMapEnd(false)
    , m_TimeSinceStart(0.0)
    , m_DurationMap()
    {
    }

    WaningEffectMapAbstract::~WaningEffectMapAbstract()
    {
    }


    json::QuickBuilder WaningEffectMapAbstract::GetSchema()
    {
        return json::QuickBuilder( jsonSchemaBase );
    }

    bool WaningEffectMapAbstract::Configure( const Configuration * pInputJson )
    {
        initConfigTypeMap( "Initial_Effect",               &m_EffectCurrent,          WEE_Initial_Effect_DESC_TEXT,                0, 1, 1);
        initConfigTypeMap( "Expire_At_Durability_Map_End", &m_ExpireAtDurationMapEnd, WEM_Expire_At_Durability_Map_End_DESC_TEXT, false );

        initConfigComplexType( "Durability_Map", &m_DurationMap, WEM_Durability_Map_End_DESC_TEXT );

        bool ret = JsonConfigurable::Configure(pInputJson);
        if( ret )
        {
            m_EffectOriginal = m_EffectCurrent;
        }
        return ret;
    }

    void  WaningEffectMapAbstract::Update(float dt)
    {
        float multiplier = GetMultiplier( m_TimeSinceStart );

        m_EffectCurrent = multiplier * m_EffectOriginal;

        if( m_ExpireAtDurationMapEnd )
        {
            m_Expired = m_DurationMap.isAtEnd( m_TimeSinceStart );
        }
        m_TimeSinceStart += dt;
    }

    float WaningEffectMapAbstract::Current() const
    {
        return m_EffectCurrent;
    }

    bool WaningEffectMapAbstract::Expired() const
    {
        return m_Expired;
    }

    void WaningEffectMapAbstract::serialize( IArchive& ar, WaningEffectMapAbstract* obj )
    {
        WaningEffectMapAbstract& effect = *obj;
        ar.labelElement( "m_Expired"                ) & effect.m_Expired;
        ar.labelElement( "m_EffectOriginal"         ) & effect.m_EffectOriginal;
        ar.labelElement( "m_EffectCurrent"          ) & effect.m_EffectCurrent;
        ar.labelElement( "m_ExpireAtDurationMapEnd" ) & effect.m_ExpireAtDurationMapEnd;
        ar.labelElement( "m_TimeSinceStart"         ) & effect.m_TimeSinceStart;
        ar.labelElement( "m_DurationMap"            ) & effect.m_DurationMap;
    }

    // ------------------------------------------------------------------------
    // --- WaningEffectMapLinear
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(WaningEffectMapLinear)
    BEGIN_QUERY_INTERFACE_DERIVED(WaningEffectMapLinear, WaningEffectMapAbstract)
    END_QUERY_INTERFACE_DERIVED(WaningEffectMapLinear, WaningEffectMapAbstract)

    REGISTER_SERIALIZABLE(WaningEffectMapLinear);


    WaningEffectMapLinear::WaningEffectMapLinear()
    : WaningEffectMapAbstract()
    {
    }

    WaningEffectMapLinear::~WaningEffectMapLinear()
    {
    }

    float WaningEffectMapLinear::GetMultiplier( float timeSinceStart ) const
    {
        return m_DurationMap.getValueLinearInterpolation( timeSinceStart );
    }

    void WaningEffectMapLinear::serialize( IArchive& ar, WaningEffectMapLinear* obj )
    {
        WaningEffectMapAbstract::serialize( ar, obj );
    }

    // ------------------------------------------------------------------------
    // --- WaningEffectMapPiecewise
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(WaningEffectMapPiecewise)
    BEGIN_QUERY_INTERFACE_DERIVED(WaningEffectMapPiecewise, WaningEffectMapAbstract)
    END_QUERY_INTERFACE_DERIVED(WaningEffectMapPiecewise, WaningEffectMapAbstract)

    REGISTER_SERIALIZABLE(WaningEffectMapPiecewise);


    WaningEffectMapPiecewise::WaningEffectMapPiecewise()
    : WaningEffectMapAbstract()
    {
    }

    WaningEffectMapPiecewise::~WaningEffectMapPiecewise()
    {
    }

    float WaningEffectMapPiecewise::GetMultiplier( float timeSinceVaccination ) const
    {
        return m_DurationMap.getValuePiecewiseConstant( timeSinceVaccination );
    }

    void WaningEffectMapPiecewise::serialize( IArchive& ar, WaningEffectMapPiecewise* obj )
    {
        WaningEffectMapAbstract::serialize( ar, obj );
    }
}
