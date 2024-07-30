
#pragma once

#include "ISerializable.h"
#include "MathFunctions.h"
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