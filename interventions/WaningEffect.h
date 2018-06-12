/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <map>

#include "IWaningEffect.h"
#include "ISerializable.h"
#include "Configuration.h"
#include "Configure.h"
#include "FactorySupport.h"

namespace Kernel
{
    // --------------------------- WaningEffectConstant ---------------------------
    class IDMAPI WaningEffectConstant : public IWaningEffect, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectConstant, IWaningEffect)

    public:
        DECLARE_CONFIGURED(WaningEffectConstant)
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        WaningEffectConstant();
        WaningEffectConstant( const WaningEffectConstant& rOrig );
        virtual ~WaningEffectConstant() {};

        virtual IWaningEffect* Clone() override;
        virtual void  Update(float dt) override;
        virtual void  SetCurrentTime(float dt) override {};
        virtual float Current() const override;
        virtual bool  Expired() const override { return false; };
        virtual void SetContextTo( IIndividualHumanContext *context ) override {};
        virtual void  SetInitial(float newVal) override;

    protected:
        float currentEffect;
        bool usingDefault;
        const int notSetByUser = -1;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE(WaningEffectConstant);
#pragma warning( pop )
    };

    // --------------------------- WaningEffectExponential ---------------------------
    class IDMAPI WaningEffectExponential : public WaningEffectConstant
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectExponential, IWaningEffect)

    public:
        DECLARE_QUERY_INTERFACE()

        WaningEffectExponential();
        WaningEffectExponential( const WaningEffectExponential& rOrig );
        virtual ~WaningEffectExponential() {};

        virtual bool Configure( const Configuration *config ) override;
        virtual IWaningEffect* Clone() override;
        virtual void  Update(float dt) override;

    protected:
        float decayTimeConstant;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE(WaningEffectExponential);
#pragma warning( pop )
    };

    // --------------------------- WaningEffectBox ---------------------------
    class IDMAPI WaningEffectBox : public WaningEffectConstant
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectBox, IWaningEffect)

    public:
        DECLARE_QUERY_INTERFACE()

        WaningEffectBox();
        WaningEffectBox( const WaningEffectBox& rOrig );
        virtual ~WaningEffectBox() {};

        virtual bool Configure( const Configuration *config ) override;
        virtual IWaningEffect* Clone() override;
        virtual void  Update(float dt) override;

    protected:
        float boxDuration;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE(WaningEffectBox);
#pragma warning( pop )
    };

    // --------------------------- WaningEffectBoxExponential ---------------------------
    class IDMAPI WaningEffectBoxExponential : public WaningEffectConstant
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectBoxExponential, IWaningEffect)

    public:
        DECLARE_QUERY_INTERFACE()

        WaningEffectBoxExponential();
        WaningEffectBoxExponential( const WaningEffectBoxExponential& rOrig );
        virtual ~WaningEffectBoxExponential() {};

        virtual bool Configure( const Configuration *config ) override;
        virtual IWaningEffect* Clone() override;
        virtual void  Update(float dt) override;

    protected:
        float boxDuration;
        float decayTimeConstant;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE(WaningEffectBoxExponential);
#pragma warning( pop )
    };
}
