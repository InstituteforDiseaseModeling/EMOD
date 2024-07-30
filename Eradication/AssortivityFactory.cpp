
#include "stdafx.h"
#include "AssortivityFactory.h"

#ifndef DISABLE_HIV
#include "AssortivityHIV.h"
#else
#include "Assortivity.h"
#endif

using namespace Kernel;

IAssortivity* AssortivityFactory::CreateAssortivity( RelationshipType::Enum relType, RANDOMBASE* prng )
{
    IAssortivity* pa = nullptr ;
#ifndef DISABLE_HIV
    pa = new AssortivityHIV( relType, prng );
#else
    pa = new Assortivity( relType, prng );
#endif
    return pa ;
}
