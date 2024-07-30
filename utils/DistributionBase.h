
#pragma once

#include "Configure.h"
#include "MathFunctions.h"
#include "ISerializable.h"
#include "IDistribution.h"

namespace Kernel
{
    class DistributionBase: public IDistribution
    {
    public:
        DistributionBase();
        DistributionBase( const DistributionBase& master );

        virtual void SetParameters( double param1, double param2, double param3 ) override;
        virtual DistributionFunction::Enum GetType() const override;
        virtual IPiecewiseDistribution* GetIPiecewiseDistribution() override;
        float GetParam1() const override;
        float GetParam2() const override;
        float GetParam3() const override;

    protected:
        DistributionFunction::Enum type;
        float m_Param1;
        float m_Param2;
        float m_Param3;
        
        static void serialize( IArchive& ar, DistributionBase* obj );
    };
}