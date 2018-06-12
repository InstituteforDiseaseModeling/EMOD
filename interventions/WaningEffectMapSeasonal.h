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
    class IDMAPI WaningEffectMapLinearSeasonal : public WaningEffectMapLinear
    {
        DECLARE_FACTORY_REGISTERED_EXPORT( WaningEffectFactory, WaningEffectMapLinearSeasonal, IWaningEffect )
        DECLARE_QUERY_INTERFACE()
    public:
        WaningEffectMapLinearSeasonal();
        WaningEffectMapLinearSeasonal( const WaningEffectMapLinearSeasonal& rOrig );
        virtual ~WaningEffectMapLinearSeasonal();
        virtual IWaningEffect* Clone() override;

        virtual void  Update( float dt ) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;

    protected:
        virtual bool ConfigureExpiration( const Configuration* config ) override;
        virtual bool ConfigureReferenceTimer( const Configuration* config ) override;

        IIndividualHumanContext* m_Parent;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE( WaningEffectMapLinearSeasonal );
#pragma warning( pop )
    };
}