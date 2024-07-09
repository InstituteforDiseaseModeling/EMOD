
#include "stdafx.h"

#include "IWaningEffect.h"
#include "ObjectFactoryTemplates.h"

SETUP_LOGGING( "WaningEffectFactory" )

namespace Kernel
{
    WaningEffectFactory* WaningEffectFactory::_instance = nullptr;

    template WaningEffectFactory* ObjectFactory<IWaningEffect, WaningEffectFactory>::getInstance();
}