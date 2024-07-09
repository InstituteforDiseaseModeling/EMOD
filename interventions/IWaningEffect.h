
#pragma once

#include <string>
#include <map>

#include "ISerializable.h"
#include "Configuration.h"
#include "Configure.h"
#include "FactorySupport.h"
#include "ObjectFactory.h"

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

    // WaningEffect classes implement this interface if their WaningEffect is not really
    // updated via time but a counter instead.  For example, a WaningEffect that is count
    // based might be used to model the probability an individual takes a particular dose
    // of a medication.
    struct IWaningEffectCount : ISupports
    {
        // Unlike Update( float dt ), this method sets the count before the current
        // effect is updated.  The effect does not increment or add this value.
        // It just sets it.
        virtual void SetCount( uint32_t numCounts ) = 0;

        // Return true if this effect is configured correctly given
        // the maximum number counted.
        virtual bool IsValidConfiguration( uint32_t maxCount ) const = 0;
    };

    typedef std::map<std::string, IWaningEffect*> waning_effects_t;

    // --------------------------- WaningEffectFactory ---------------------------
    class WaningEffectFactory : public ObjectFactory<IWaningEffect,WaningEffectFactory>
    {
    };
}
