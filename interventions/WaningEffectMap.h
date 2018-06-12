/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <map>

#include "WaningEffect.h"
#include "InterpolatedValueMap.h"

namespace Kernel
{
    class IDMAPI WaningEffectMapAbstract : public WaningEffectConstant
    {
    public:
        DECLARE_QUERY_INTERFACE()

        virtual ~WaningEffectMapAbstract();
        virtual bool Configure( const Configuration *config ) override;
        virtual void  Update(float dt) override;
        virtual void  SetCurrentTime(float dt) override; // 'fast-forward'
        virtual bool  Expired() const override;
        virtual void  SetInitial(float newVal) override;

        virtual float GetMultiplier( float timeSinceStart ) const = 0;

    protected:
        WaningEffectMapAbstract( float maxTime = 999999.0f );
        WaningEffectMapAbstract( float minTime, float maxTime, float minValue, float maxValue );
        WaningEffectMapAbstract( const WaningEffectMapAbstract& rOrig );

        virtual void UpdateEffect();
        virtual bool ConfigureExpiration( const Configuration* config );
        virtual bool ConfigureReferenceTimer( const Configuration* config );

        bool  m_Expired;
        float m_EffectOriginal;
        bool  m_ExpireAtDurationMapEnd;
        NonNegativeFloat m_TimeSinceStart;
        int   m_RefTime;
        InterpolatedValueMap m_DurationMap;

        static void serialize( IArchive&, WaningEffectMapAbstract*);
    };

    class IDMAPI WaningEffectMapLinear : public WaningEffectMapAbstract
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectMapLinear, IWaningEffect)
    public:
        DECLARE_QUERY_INTERFACE()

        WaningEffectMapLinear( float maxTime = 999999.0f );
        WaningEffectMapLinear( const WaningEffectMapLinear& rOrig );
        virtual ~WaningEffectMapLinear();
        virtual IWaningEffect* Clone() override;

        virtual float GetMultiplier( float timeSinceStart ) const override;

    protected:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE(WaningEffectMapLinear);
#pragma warning( pop )
    };

    class IDMAPI WaningEffectMapPiecewise : public WaningEffectMapAbstract
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectMapPiecewise, IWaningEffect)
    public:
        DECLARE_QUERY_INTERFACE()

        WaningEffectMapPiecewise();
        WaningEffectMapPiecewise( float minTime, float maxTime, float minValue, float maxValue );
        WaningEffectMapPiecewise( const WaningEffectMapPiecewise& rOrig );
        virtual ~WaningEffectMapPiecewise();
        virtual IWaningEffect* Clone() override;

        virtual float GetMultiplier( float timeSinceStart ) const override;

    protected:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE(WaningEffectMapPiecewise);
#pragma warning( pop )
    };
}
