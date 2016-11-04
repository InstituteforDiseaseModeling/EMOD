/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

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

    REGISTER_SERIALIZABLE(WaningEffectConstant);

    void WaningEffectConstant::serialize(IArchive& ar, WaningEffectConstant* obj)
    {
        WaningEffectConstant& effect = *obj;
        ar.labelElement("currentEffect") & effect.currentEffect;
    }
}
