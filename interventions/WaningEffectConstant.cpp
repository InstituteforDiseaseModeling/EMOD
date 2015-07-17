/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "WaningEffect.h"
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"

static const char* _module = "WaningEffectConstant";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(WaningEffectConstant)
    IMPL_QUERY_INTERFACE2(WaningEffectConstant, IWaningEffect, IConfigurable)

    json::QuickBuilder WaningEffectConstant::GetSchema()
    {
        return json::QuickBuilder( jsonSchemaBase );
    }

    bool WaningEffectConstant::Configure( const Configuration * pInputJson )
    {
        initConfigTypeMap("Initial_Effect", &currentEffect, WEC_Initial_Effect_DESC_TEXT, 0, 1, 1);
        return JsonConfigurable::Configure(pInputJson);
    }

    void  WaningEffectConstant::Update(float dt)
    {
        // constant effect = nothing to update
        return;
    }

    float WaningEffectConstant::Current() const
    {
        return currentEffect;
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT(Kernel::WaningEffectConstant)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Kernel:IWaningEffect);

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, WaningEffectConstant& we, const unsigned int v)
    {
        boost::serialization::void_cast_register<WaningEffectConstant, IWaningEffect>();
        ar & we.currentEffect;
    }
}
#endif
