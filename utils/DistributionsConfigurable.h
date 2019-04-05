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
#include "Distributions.h"
#include <string>

namespace Kernel
{   
    class DistributionConstantConfigurable : public DistributionConstant
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_SERIALIZABLE( DistributionConstantConfigurable )
    public:
        DistributionConstantConfigurable( );
        ~DistributionConstantConfigurable();
        virtual bool Configure( JsonConfigurable* pParent, std::string& parameter, const Configuration* config );
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionExponentialConfigurable : public DistributionExponential
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_SERIALIZABLE( DistributionExponentialConfigurable )
    public:
        DistributionExponentialConfigurable( );
        ~DistributionExponentialConfigurable();
        virtual bool Configure( JsonConfigurable* pParent, std::string& param_name, const Configuration* config );
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionGaussianConfigurable : public DistributionGaussian
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_SERIALIZABLE( DistributionGaussianConfigurable )
    public:
        DistributionGaussianConfigurable( );
        ~DistributionGaussianConfigurable();
        virtual bool Configure( JsonConfigurable * pParent, std::string & param_name, const Configuration * config );
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionPoissonConfigurable : public DistributionPoisson
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_SERIALIZABLE( DistributionPoissonConfigurable )
    public:
        DistributionPoissonConfigurable( );
        ~DistributionPoissonConfigurable();
        virtual bool Configure( JsonConfigurable* pParent, std::string& param_name, const Configuration* config );
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionLogNormalConfigurable : public DistributionLogNormal
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_SERIALIZABLE( DistributionLogNormalConfigurable )
    public:
        DistributionLogNormalConfigurable( );
        ~DistributionLogNormalConfigurable();
        virtual bool Configure( JsonConfigurable* pParent, std::string& param_name, const Configuration* config);
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionWeibullConfigurable : public DistributionWeibull
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_SERIALIZABLE( DistributionWeibullConfigurable )
    public:
        DistributionWeibullConfigurable( );
        ~DistributionWeibullConfigurable();
        virtual bool Configure( JsonConfigurable* pParent, std::string& parameter, const Configuration* config );
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionDualConstantConfigurable : public DistributionDualConstant
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_SERIALIZABLE( DistributionDualConstantConfigurable )
    public:
        DistributionDualConstantConfigurable( );
        ~DistributionDualConstantConfigurable();
        virtual bool Configure( JsonConfigurable* pParent, std::string& parameter, const Configuration* config );
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionDualExponentialConfigurable : public DistributionDualExponential
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_SERIALIZABLE( DistributionDualExponentialConfigurable )
    public:
        DistributionDualExponentialConfigurable( );
        ~DistributionDualExponentialConfigurable();
        virtual bool Configure( JsonConfigurable* pParent, std::string& parameter, const Configuration* config );
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };

    class DistributionUniformConfigurable : public DistributionUniform
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_SERIALIZABLE( DistributionUniformConfigurable )
    public:
        DistributionUniformConfigurable( );
        ~DistributionUniformConfigurable();
        virtual bool Configure( JsonConfigurable* pParent, std::string& parameter, const Configuration* config );
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
    };



    //******************  PiecewiseDistribution ******************
    class DistributionPiecewiseConstantConfigurable : public DistributionPiecewiseConstant
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        DistributionPiecewiseConstantConfigurable();
        ~DistributionPiecewiseConstantConfigurable();
        virtual bool Configure( JsonConfigurable* pParent, std::string& param_base_name, const Configuration* config );
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
        DECLARE_SERIALIZABLE( DistributionPiecewiseConstantConfigurable );
    };

    class DistributionPiecewiseLinearConfigurable : public DistributionPiecewiseLinear
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        DistributionPiecewiseLinearConfigurable();
        ~DistributionPiecewiseLinearConfigurable();
        virtual bool Configure( JsonConfigurable* pParent, std::string& param_base_name, const Configuration* config );
        virtual IDistribution* Clone() const override;
        DECLARE_QUERY_INTERFACE()
        DECLARE_SERIALIZABLE( DistributionPiecewiseLinearConfigurable );
    };
    
}