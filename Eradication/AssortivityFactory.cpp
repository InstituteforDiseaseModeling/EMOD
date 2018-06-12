/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
