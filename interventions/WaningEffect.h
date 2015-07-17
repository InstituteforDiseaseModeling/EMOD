/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <map>

#include "BoostLibWrapper.h"
#include "ISupports.h"
#include "Configuration.h"
#include "Configure.h"
#include "FactorySupport.h"

namespace Kernel
{
    struct IWaningEffect : public ISupports
    {
        virtual void  Update(float dt) = 0;
        virtual float Current() const  = 0;
    };

    typedef std::map<std::string, IWaningEffect*> waning_effects_t;

    class IWaningEffectFactory
    {
    public:
        virtual void Register(string classname, instantiator_function_t _if) = 0;
        virtual json::QuickBuilder GetSchema() = 0;
    };

    // --------------------------- WaningEffectFactory ---------------------------
    class WaningEffectFactory : public IWaningEffectFactory
    {
    public:
        static IWaningEffectFactory * getInstance() { return _instance ? _instance : _instance = new WaningEffectFactory(); }

        static IWaningEffect* CreateInstance(const Configuration * config)
        {
            return CreateInstanceFromSpecs<IWaningEffect>(config, getRegisteredClasses(), true);
        }
        void Register(string classname, instantiator_function_t _if)  {  getRegisteredClasses()[classname] = _if;  }
        json::QuickBuilder GetSchema();

    protected:
        static json::Object campaignSchema;
        static support_spec_map_t& getRegisteredClasses() { static support_spec_map_t registered_classes; return registered_classes; }

    private:
        static IWaningEffectFactory * _instance;
    };

    // --------------------------- WaningEffectConstant ---------------------------
    class WaningEffectConstant : public IWaningEffect, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectConstant, IWaningEffect)

    public:
        DECLARE_CONFIGURED(WaningEffectConstant)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual void  Update(float dt);
        virtual float Current() const;

    protected:
        float currentEffect;

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, WaningEffectConstant& we, const unsigned int v);
#endif
    };

    // --------------------------- WaningEffectExponential ---------------------------
    class WaningEffectExponential : public IWaningEffect, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectExponential, IWaningEffect)

    public:
        DECLARE_CONFIGURED(WaningEffectExponential)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual void  Update(float dt);
        virtual float Current() const;

    protected:
        float currentEffect;
        float decayTimeConstant;

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, WaningEffectExponential& we, const unsigned int v);
#endif
    };

    // --------------------------- WaningEffectBox ---------------------------
    class WaningEffectBox : public IWaningEffect, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectBox, IWaningEffect)

    public:
        DECLARE_CONFIGURED(WaningEffectBox)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual void  Update(float dt);
        virtual float Current() const;

    protected:
        float currentEffect;
        float boxDuration;

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, WaningEffectBox& we, const unsigned int v);
#endif
    };

    // --------------------------- WaningEffectBoxExponential ---------------------------
    class WaningEffectBoxExponential : public IWaningEffect, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(WaningEffectFactory, WaningEffectBoxExponential, IWaningEffect)

    public:
        DECLARE_CONFIGURED(WaningEffectBoxExponential)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual void  Update(float dt);
        virtual float Current() const;

    protected:
        float currentEffect;
        float boxDuration;
        float decayTimeConstant;

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, WaningEffectBoxExponential& we, const unsigned int v);
#endif
    };
}