/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <map>

#include "ISerializable.h"
#include "Configuration.h"
#include "Configure.h"
#include "FactorySupport.h"

namespace Kernel
{
    struct IIndividualHumanContext;

    struct IDMAPI IWaningEffect : ISerializable
    {
        virtual IWaningEffect* Clone() = 0;
        virtual void  Update(float dt) = 0;
        virtual float Current() const  = 0;
        virtual bool  Expired() const  = 0;
        virtual void  SetContextTo( IIndividualHumanContext *context ) = 0;
        virtual void  SetInitial(float newVal) = 0;
        virtual void  SetCurrentTime(float dt) = 0;
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

        static IWaningEffect* CreateInstance( const WaningConfig& rWaningConfig )
        {
            auto p_config = Configuration::CopyFromElement( rWaningConfig._json, "campaign" );
            IWaningEffect* p_effect = CreateInstanceFromSpecs<IWaningEffect>( p_config, getRegisteredClasses(), true);
            delete p_config;
            return p_effect;
        }

        void Register(string classname, instantiator_function_t _if)  {  getRegisteredClasses()[classname] = _if;  }
        json::QuickBuilder GetSchema();

    protected:
        static json::Object campaignSchema;
        static support_spec_map_t& getRegisteredClasses() { static support_spec_map_t registered_classes; return registered_classes; }

    private:
        static IWaningEffectFactory * _instance;
    };
}
