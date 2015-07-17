/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "WaningEffect.h"
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"

static const char* _module = "WaningEffectBox";

namespace Kernel
{
    // --------------------------- WaningEffectBox ---------------------------
    IMPLEMENT_FACTORY_REGISTERED(WaningEffectBox)
    IMPL_QUERY_INTERFACE2(WaningEffectBox, IWaningEffect, IConfigurable)

    json::QuickBuilder WaningEffectBox::GetSchema()
    {
        return json::QuickBuilder( jsonSchemaBase );
    }

    bool WaningEffectBox::Configure( const Configuration * pInputJson )
    {
        initConfigTypeMap("Initial_Effect", &currentEffect, WEB_Initial_Effect_DESC_TEXT, 0, 1,      1);
        initConfigTypeMap("Box_Duration",   &boxDuration,   WEB_Box_Duration_DESC_TEXT,   0, 100000, 100);
        return JsonConfigurable::Configure(pInputJson);
    }

    void  WaningEffectBox::Update(float dt)
    {
        boxDuration -= dt;
        if ( boxDuration < 0 )
        {
            currentEffect = 0;
        }
    }

    float WaningEffectBox::Current() const
    {
        return currentEffect;
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT(Kernel::WaningEffectBox)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Kernel:IWaningEffect);

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, WaningEffectBox& we, const unsigned int v)
    {
        boost::serialization::void_cast_register<WaningEffectBox, IWaningEffect>();
        ar & we.currentEffect;
        ar & we.boxDuration;
    }
}
#endif
