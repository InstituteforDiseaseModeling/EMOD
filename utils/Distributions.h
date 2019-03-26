/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "stdafx.h"
#include "Configuration.h"
#include "Configure.h"
#include "DistributionBase.h"
#include "PiecewiseDistributionBase.h"

namespace Kernel
{
    
    class DistributionConstant : public DistributionBase
    {
        DECLARE_SERIALIZABLE( DistributionConstant )
    public:
        DistributionConstant();
        ~DistributionConstant();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionExponential : public DistributionBase
    {
        DECLARE_SERIALIZABLE( DistributionExponential )
    public:
        DistributionExponential();
        ~DistributionExponential();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual void SetParameters( double param1, double param2, double param3 ) override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionGaussian : public DistributionBase
    {
        DECLARE_SERIALIZABLE( DistributionGaussian )
    public:
        DistributionGaussian();
        ~DistributionGaussian();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionPoisson : public DistributionBase
    {
        DECLARE_SERIALIZABLE( DistributionPoisson )
    public:
        DistributionPoisson();
        ~DistributionPoisson();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionLogNormal : public DistributionBase
    {
        DECLARE_SERIALIZABLE( DistributionLogNormal )
    public:
        DistributionLogNormal();
        ~DistributionLogNormal();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionWeibull : public DistributionBase
    {
        DECLARE_SERIALIZABLE( DistributionWeibull )
    public:
        DistributionWeibull();
        ~DistributionWeibull();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionDualConstant : public DistributionBase
    {
        DECLARE_SERIALIZABLE( DistributionDualConstant )
    public:
        DistributionDualConstant();
        ~DistributionDualConstant();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionDualExponential : public DistributionBase
    {
        DECLARE_SERIALIZABLE( DistributionDualExponential )
    public:
        DistributionDualExponential();
        ~DistributionDualExponential();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionUniform : public DistributionBase
    {
        DECLARE_SERIALIZABLE( DistributionUniform )
    public:
        DistributionUniform();
        ~DistributionUniform();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    /******************  PiecewiseDistribution ******************/
    class DistributionPiecewiseConstant : public PiecewiseDistributionBase
    {
        DECLARE_SERIALIZABLE( DistributionPiecewiseConstant )
    public:
        DistributionPiecewiseConstant();
        ~DistributionPiecewiseConstant();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionPiecewiseLinear : public PiecewiseDistributionBase
    {        
        DECLARE_SERIALIZABLE( DistributionPiecewiseLinear )
    public:
        DistributionPiecewiseLinear();
        ~DistributionPiecewiseLinear();
        virtual float Calculate( RANDOMBASE* pRNG ) const override;
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };
}