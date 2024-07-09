
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
        virtual void  SetContextTo( IIndividualHumanContext *context ) override;
        virtual void  Update(float dt) override;
        virtual bool  Expired() const override;

    protected:
        float m_ExpectedDiscardTime = 0;
        float m_DiscardCounter;

        DECLARE_SERIALIZABLE(WaningEffectRandomBox);
    };
}