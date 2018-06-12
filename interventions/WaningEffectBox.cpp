/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "WaningEffect.h"
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"

SETUP_LOGGING( "WaningEffectBox" )

namespace Kernel
{
    // --------------------------- WaningEffectBox ---------------------------
    IMPLEMENT_FACTORY_REGISTERED(WaningEffectBox)
    IMPL_QUERY_INTERFACE2(WaningEffectBox, IWaningEffect, IConfigurable)

    WaningEffectBox::WaningEffectBox()
    : WaningEffectConstant()
    , boxDuration( 0.0f )
    {
    }

    WaningEffectBox::WaningEffectBox( const WaningEffectBox& rOrig )
    : WaningEffectConstant( rOrig )
    , boxDuration( rOrig.boxDuration )
    {
    }

    IWaningEffect* WaningEffectBox::Clone()
    {
        return new WaningEffectBox( *this );
    }

    bool WaningEffectBox::Configure( const Configuration * pInputJson )
    {
        initConfigTypeMap("Box_Duration",   &boxDuration,   WEB_Box_Duration_DESC_TEXT,   0, 100000, 100);

        bool ret = WaningEffectConstant::Configure(pInputJson);
        LOG_VALID_F( "WaningEffectBox configured and initialized with effect = %f and boxDuration = %f.\n", currentEffect, boxDuration );
        return ret;
    }

    void  WaningEffectBox::Update(float dt)
    {
        boxDuration -= dt;
        if ( boxDuration < 0 )
        {
            currentEffect = 0;
        }
        LOG_VALID_F( "boxDuration = %f, currentEffect = %f.\n", boxDuration, currentEffect );
    }

    REGISTER_SERIALIZABLE(WaningEffectBox);

    void WaningEffectBox::serialize(IArchive& ar, WaningEffectBox* obj)
    {
        WaningEffectConstant::serialize( ar, obj );
        WaningEffectBox& effect = *obj;
        ar.labelElement("boxDuration") & effect.boxDuration;
    }
}
