/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Configure.h"
#include "MathFunctions.h"
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
        IPiecewiseDistribution* GetIPiecewiseDistribution();

    protected:
        static void serialize( IArchive& ar, PiecewiseDistributionBase* obj );
    
        InterpolatedValueMap m_interpolatedValueMap;
        NonNegativeFloat m_x;
    };
}