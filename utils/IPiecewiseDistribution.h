/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Configure.h"
#include "IdmApi.h"
#include "ISerializable.h"
#include "MathFunctions.h"
#include "InterpolatedValueMap.h"

namespace Kernel
{
    class IPiecewiseDistribution
    {
    public:
        virtual void SetX( NonNegativeFloat x ) = 0;
    };
}