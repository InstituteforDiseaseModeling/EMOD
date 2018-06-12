/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <map>

#include "WaningEffect.h"

namespace Kernel
{
    class WaningEffectRandomBox : public WaningEffectConstant
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectRandomBox, IWaningEffect)

    public:
        //DECLARE_CONFIGURED(WaningEffectRandomBox)
        DECLARE_QUERY_INTERFACE()

        WaningEffectRandomBox();
        WaningEffectRandomBox( const WaningEffectRandomBox& master );
        virtual ~WaningEffectRandomBox();
        virtual bool Configure( const Configuration *config ) override;
        virtual IWaningEffect* Clone() override;
        virtual void  Update(float dt) override;
        virtual bool  Expired() const override;

    protected:
        float m_DiscardCounter;

        DECLARE_SERIALIZABLE(WaningEffectRandomBox);
    };
}