/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Configure.h"
#include "MathFunctions.h"
#include "IdmApi.h"
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
        IPiecewiseDistribution* GetIPiecewiseDistribution() override;
        float GetParam1() const override;
        float GetParam2() const override;
        float GetParam3() const override;

    protected:
        DistributionFunction::Enum type;
        static void serialize( IArchive& ar, DistributionBase* obj );
        float m_Param1;
        float m_Param2;
        float m_Param3;
    };
}