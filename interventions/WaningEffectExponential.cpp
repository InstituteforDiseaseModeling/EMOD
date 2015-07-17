/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "WaningEffect.h"
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"

static const char* _module = "WaningEffectExponential";

namespace Kernel
{
    // --------------------------- WaningEffectExponential ---------------------------
    IMPLEMENT_FACTORY_REGISTERED(WaningEffectExponential)
    IMPL_QUERY_INTERFACE2(WaningEffectExponential, IWaningEffect, IConfigurable)

    json::QuickBuilder WaningEffectExponential::GetSchema()
    {
        return json::QuickBuilder( jsonSchemaBase );
    }

    bool WaningEffectExponential::Configure( const Configuration * pInputJson )
    {
        initConfigTypeMap("Initial_Effect",      &currentEffect,     WEE_Initial_Effect_DESC_TEXT,       0, 1, 1);
        initConfigTypeMap("Decay_Time_Constant", &decayTimeConstant, WEE_Decay_Time_Constant_DESC_TEXT, 0, 100000, 100);
        return JsonConfigurable::Configure(pInputJson);
    }

    void  WaningEffectExponential::Update(float dt)
    {
        if ( decayTimeConstant > dt )
        {
            currentEffect *= (1 - dt/decayTimeConstant);
        }
        else
        {
            currentEffect = 0;
        }
    }

    float WaningEffectExponential::Current() const
    {
        return currentEffect;
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT(Kernel::WaningEffectExponential)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Kernel:IWaningEffect);

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, WaningEffectExponential& we, const unsigned int v)
    {
        boost::serialization::void_cast_register<WaningEffectExponential, IWaningEffect>();
        ar & we.currentEffect;
        ar & we.decayTimeConstant;
    }
}
#endif
