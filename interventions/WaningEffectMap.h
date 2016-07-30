/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <map>

#include "IWaningEffect.h"
#include "ISerializable.h"
#include "Configuration.h"
#include "Configure.h"
#include "FactorySupport.h"
#include "InterpolatedValueMap.h"

namespace Kernel
{
    class IDMAPI WaningEffectMapAbstract : public IWaningEffect, public JsonConfigurable
    {
    public:
        DECLARE_CONFIGURED(WaningEffectExponential)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        WaningEffectMapAbstract();
        virtual ~WaningEffectMapAbstract();
        virtual void  Update(float dt) override;
        virtual float Current() const override;
        virtual bool  Expired() const override;

        virtual float GetMultiplier( float timeSinceStart ) const = 0;

    protected:

        bool  m_Expired;
        float m_EffectOriginal;
        float m_EffectCurrent;
        bool  m_ExpireAtDurationMapEnd;
        float m_TimeSinceStart;
        InterpolatedValueMap m_DurationMap;

        static void serialize( IArchive&, WaningEffectMapAbstract*);
    };

    class IDMAPI WaningEffectMapLinear : public WaningEffectMapAbstract
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectMapLinear, IWaningEffect)
        DECLARE_QUERY_INTERFACE()
    public:
        WaningEffectMapLinear();
        virtual ~WaningEffectMapLinear();

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
        DECLARE_QUERY_INTERFACE()
    public:
        WaningEffectMapPiecewise();
        virtual ~WaningEffectMapPiecewise();

        virtual float GetMultiplier( float timeSinceStart ) const override;

    protected:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE(WaningEffectMapPiecewise);
#pragma warning( pop )
    };
}