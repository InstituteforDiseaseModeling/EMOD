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
#include "IPiecewiseDistribution.h"

namespace Kernel
{
    class RANDOMBASE;
    
    struct IDistribution : public ISerializable
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        virtual float Calculate( RANDOMBASE* pRNG ) const = 0;
        virtual void SetParameters( double param1, double param2, double param3 ) = 0;
        virtual DistributionFunction::Enum GetType() const = 0;
        virtual IPiecewiseDistribution* GetIPiecewiseDistribution() = 0;
        virtual float GetParam1() const = 0;
        virtual float GetParam2() const = 0;
        virtual float GetParam3() const = 0;
        virtual IDistribution* Clone() const = 0;
    };
}