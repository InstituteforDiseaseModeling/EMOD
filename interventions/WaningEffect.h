/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
    class WaningEffectConstant : public IWaningEffect, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectConstant, IWaningEffect)

    public:
        DECLARE_CONFIGURED(WaningEffectConstant)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual ~WaningEffectConstant() {};
        virtual void  Update(float dt) override;
        virtual float Current() const override;
        virtual bool  Expired() const override { return false; };

    protected:
        float currentEffect;

        DECLARE_SERIALIZABLE(WaningEffectConstant);
    };

    // --------------------------- WaningEffectExponential ---------------------------
    class WaningEffectExponential : public IWaningEffect, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectExponential, IWaningEffect)

    public:
        DECLARE_CONFIGURED(WaningEffectExponential)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual ~WaningEffectExponential() {};
        virtual void  Update(float dt) override;
        virtual float Current() const override;
        virtual bool  Expired() const override { return false; };

    protected:
        float currentEffect;
        float decayTimeConstant;

        DECLARE_SERIALIZABLE(WaningEffectExponential);
    };

    // --------------------------- WaningEffectBox ---------------------------
    class WaningEffectBox : public IWaningEffect, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectBox, IWaningEffect)

    public:
        DECLARE_CONFIGURED(WaningEffectBox)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual ~WaningEffectBox() {};
        virtual void  Update(float dt) override;
        virtual float Current() const override;
        virtual bool  Expired() const override { return false; };

    protected:
        float currentEffect;
        float boxDuration;

        DECLARE_SERIALIZABLE(WaningEffectBox);
    };

    // --------------------------- WaningEffectBoxExponential ---------------------------
    class WaningEffectBoxExponential : public IWaningEffect, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectBoxExponential, IWaningEffect)

    public:
        DECLARE_CONFIGURED(WaningEffectBoxExponential)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual ~WaningEffectBoxExponential() {};
        virtual void  Update(float dt) override;
        virtual float Current() const override;
        virtual bool  Expired() const override { return false; };

    protected:
        float currentEffect;
        float boxDuration;
        float decayTimeConstant;

        DECLARE_SERIALIZABLE(WaningEffectBoxExponential);
    };
}