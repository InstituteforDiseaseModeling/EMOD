/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "WaningEffectMapAge.h"
#include "Contexts.h"
#include "IndividualEventContext.h"

SETUP_LOGGING( "WaningEffectMapAge" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- WaningEffectMapAge
    // ------------------------------------------------------------------------
    IMPLEMENT_FACTORY_REGISTERED( WaningEffectMapLinearAge )
    BEGIN_QUERY_INTERFACE_DERIVED( WaningEffectMapLinearAge, WaningEffectMapLinear )
    END_QUERY_INTERFACE_DERIVED( WaningEffectMapLinearAge, WaningEffectMapLinear )

    WaningEffectMapLinearAge::WaningEffectMapLinearAge()
        : WaningEffectMapLinear( MAX_HUMAN_AGE )
        , m_Parent( nullptr )
    {
    }

    WaningEffectMapLinearAge::WaningEffectMapLinearAge( const WaningEffectMapLinearAge& rOrig )
        : WaningEffectMapLinear( rOrig )
        , m_Parent( nullptr )
    {
    }

    WaningEffectMapLinearAge::~WaningEffectMapLinearAge()
    {
    }

    IWaningEffect* WaningEffectMapLinearAge::Clone()
    {
        return new WaningEffectMapLinearAge( *this );
    }

    bool WaningEffectMapLinearAge::ConfigureExpiration( const Configuration* config )
    {
        // disable m_ExpireAtDurationMapEnd
        return true;
    }

    bool WaningEffectMapLinearAge::ConfigureReferenceTimer( const Configuration* config )
    {
        // disable Reference_Timer
        return true;
    }

    float WaningEffectMapLinearAge::GetMultiplier( float timeSinceStart ) const
    {
        if( m_Parent == nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "SetContextTo() has not been called properly." );
        }
        float age_years = m_Parent->GetEventContext()->GetAge() / DAYSPERYEAR;
        return WaningEffectMapLinear::GetMultiplier( age_years );
    }

    void WaningEffectMapLinearAge::SetContextTo( IIndividualHumanContext *context )
    {
        m_Parent = context;
    }


    REGISTER_SERIALIZABLE( WaningEffectMapLinearAge );

    void WaningEffectMapLinearAge::serialize( IArchive& ar, WaningEffectMapLinearAge* obj )
    {
        WaningEffectMapLinear::serialize( ar, obj );
        WaningEffectMapLinearAge& effect = *obj;
    }
}
