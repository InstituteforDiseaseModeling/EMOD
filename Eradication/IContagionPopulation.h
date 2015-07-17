/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "StrainIdentity.h"
#include "ISupports.h"

namespace Kernel
{
    typedef int AntigenId;

    struct IContagionPopulation: public ISupports
    {
        virtual AntigenId GetAntigenId( void ) const = 0;
        virtual float GetTotalContagion( void ) const = 0;
        virtual void ResolveInfectingStrain( StrainIdentity* strainId ) const = 0;
    };
}
