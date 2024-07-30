
#pragma once

#include "DistributionBase.h"
#include "InterpolatedValueMap.h"
#include "IPiecewiseDistribution.h"


namespace Kernel
{
    class PiecewiseDistributionBase : public DistributionBase, public IPiecewiseDistribution
    {
    public:
        PiecewiseDistributionBase();
        PiecewiseDistributionBase( const PiecewiseDistributionBase& master );

        virtual void SetX( NonNegativeFloat x ) override;
        virtual IPiecewiseDistribution* GetIPiecewiseDistribution() override ;

    protected:    
        InterpolatedValueMap m_interpolatedValueMap;
        NonNegativeFloat m_x;
        
        static void serialize( IArchive& ar, PiecewiseDistributionBase* obj );
    };
}