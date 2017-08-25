/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "WaningEffectRandomBox.h"
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"
#include "Contexts.h"
#include "IArchive.h"

SETUP_LOGGING( "WaningEffectRandomBox" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- WaningEffectRandomBox
    // ------------------------------------------------------------------------
    IMPLEMENT_FACTORY_REGISTERED(WaningEffectRandomBox)
    IMPL_QUERY_INTERFACE2(WaningEffectRandomBox, IWaningEffect, IConfigurable)

    WaningEffectRandomBox::WaningEffectRandomBox()
    : WaningEffectConstant()
    , m_DiscardCounter(0.0)
    {
    }

    WaningEffectRandomBox::WaningEffectRandomBox( const WaningEffectRandomBox& master )
    : WaningEffectConstant( master )
    , m_DiscardCounter( master.m_DiscardCounter )
    {
    }

    WaningEffectRandomBox::~WaningEffectRandomBox()
    {
    }

    IWaningEffect* WaningEffectRandomBox::Clone()
    {
        return new WaningEffectRandomBox( *this );
    }

    bool WaningEffectRandomBox::Configure(const Configuration * pInputJson)
    {
        float expected_discard_time = 0;

        initConfigTypeMap("Expected_Discard_Time", &expected_discard_time, WERB_Expected_Discard_Time_DESC_TEXT, 0, 100000, 100);
        
        bool ret = WaningEffectConstant::Configure(pInputJson);

        if( ret && !JsonConfigurable::_dryrun )
        {
            if( expected_discard_time == 0 )
            {
                m_DiscardCounter = 0;
            }
            else
            {
                m_DiscardCounter = randgen->expdist( 1.0 / expected_discard_time );
            }
            LOG_DEBUG_F( "Discard after %0.1f days based on expected duration of %0.1f\n", m_DiscardCounter, expected_discard_time );
        }
        return ret;
    }

    void  WaningEffectRandomBox::Update(float dt)
    {
        m_DiscardCounter -= dt;
        LOG_DEBUG_F("Discard counter = %0.1f\n", m_DiscardCounter);

        if( Expired() ) 
        {
            LOG_DEBUG("Waning effect has expired.\n");
            currentEffect = 0.0f;
        }
    }

    bool WaningEffectRandomBox::Expired() const
    {
        return m_DiscardCounter <= 0.0f;
    }

    REGISTER_SERIALIZABLE(WaningEffectRandomBox);

    void WaningEffectRandomBox::serialize(IArchive& ar, WaningEffectRandomBox* obj)
    {
        WaningEffectConstant::serialize( ar, obj );
        WaningEffectRandomBox& effect = *obj;
        ar.labelElement( "m_DiscardCounter") & effect.m_DiscardCounter;
    }
}