/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "WaningEffect.h"
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"

static const char* _module = "WaningEffectBoxExponential";

namespace Kernel
{
    // --------------------------- WaningEffectBoxExponential ---------------------------
    IMPLEMENT_FACTORY_REGISTERED(WaningEffectBoxExponential)
    IMPL_QUERY_INTERFACE2(WaningEffectBoxExponential, IWaningEffect, IConfigurable)

    json::QuickBuilder WaningEffectBoxExponential::GetSchema()
    {
        return json::QuickBuilder( jsonSchemaBase );
    }

    bool WaningEffectBoxExponential::Configure( const Configuration * pInputJson )
    {
        initConfigTypeMap("Initial_Effect",      &currentEffect,     WEBE_Initial_Effect_DESC_TEXT,      0, 1, 1);
        initConfigTypeMap("Box_Duration",        &boxDuration,       WEBE_Box_Duration_DESC_TEXT,        0, 100000, 100);
        initConfigTypeMap("Decay_Time_Constant", &decayTimeConstant, WEBE_Decay_Time_Constant_DESC_TEXT, 0, 100000, 100);
        return JsonConfigurable::Configure(pInputJson);
    }

    void  WaningEffectBoxExponential::Update(float dt)
    {
        if ( boxDuration > 0 )
        {
            boxDuration -= dt;
        }
        else if ( decayTimeConstant > dt )
        {
            currentEffect *= (1 - dt/decayTimeConstant);
        }
        else
        {
            currentEffect = 0;
        }
    }

    float WaningEffectBoxExponential::Current() const
    {
        return currentEffect;
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT(Kernel::WaningEffectBoxExponential)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Kernel:IWaningEffect);

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, WaningEffectBoxExponential& we, const unsigned int v)
    {
        boost::serialization::void_cast_register<WaningEffectBoxExponential, IWaningEffect>();
        ar & we.currentEffect;
        ar & we.boxDuration;
        ar & we.decayTimeConstant;
    }
}
#endif
