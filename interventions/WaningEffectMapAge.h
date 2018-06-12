#pragma once
/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <map>

#include "WaningEffectMap.h"

namespace Kernel
{
    class IDMAPI WaningEffectMapLinearAge : public WaningEffectMapLinear
    {
        DECLARE_FACTORY_REGISTERED_EXPORT( WaningEffectFactory, WaningEffectMapLinearAge, IWaningEffect )
        DECLARE_QUERY_INTERFACE()
    public:
        WaningEffectMapLinearAge();
        WaningEffectMapLinearAge( const WaningEffectMapLinearAge& rOrig );
        virtual ~WaningEffectMapLinearAge();
        virtual IWaningEffect* Clone() override;

        virtual void SetContextTo( IIndividualHumanContext *context ) override;
        virtual float GetMultiplier( float timeSinceStart ) const override;

    protected:
        virtual bool ConfigureExpiration( const Configuration* config ) override;
        virtual bool ConfigureReferenceTimer( const Configuration* config ) override;

        IIndividualHumanContext* m_Parent;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE( WaningEffectMapLinearAge );
#pragma warning( pop )
    };
}