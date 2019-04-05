/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Distributions.h"
#include "IArchive.h"
#include "RANDOM.h"
#include "Environment.h"
#include "PiecewiseDistributionBase.h"
#include "InterpolatedValueMap.h"

SETUP_LOGGING("DurationDistributionImpl")

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( DistributionExponential )
    END_QUERY_INTERFACE_BODY( DistributionExponential )

    BEGIN_QUERY_INTERFACE_BODY( DistributionGaussian )
    END_QUERY_INTERFACE_BODY( DistributionGaussian )

    BEGIN_QUERY_INTERFACE_BODY( DistributionPoisson )
    END_QUERY_INTERFACE_BODY( DistributionPoisson )

    BEGIN_QUERY_INTERFACE_BODY( DistributionLogNormal )
    END_QUERY_INTERFACE_BODY( DistributionLogNormal )

    BEGIN_QUERY_INTERFACE_BODY( DistributionConstant )
    END_QUERY_INTERFACE_BODY( DistributionConstant )

    BEGIN_QUERY_INTERFACE_BODY( DistributionWeibull )
    END_QUERY_INTERFACE_BODY( DistributionWeibull )

    BEGIN_QUERY_INTERFACE_BODY( DistributionDualConstant )
    END_QUERY_INTERFACE_BODY( DistributionDualConstant )

    BEGIN_QUERY_INTERFACE_BODY( DistributionDualExponential )
    END_QUERY_INTERFACE_BODY( DistributionDualExponential )

    BEGIN_QUERY_INTERFACE_BODY( DistributionUniform )
    END_QUERY_INTERFACE_BODY( DistributionUniform )

    BEGIN_QUERY_INTERFACE_BODY( DistributionPiecewiseConstant )
    END_QUERY_INTERFACE_BODY( DistributionPiecewiseConstant )

    BEGIN_QUERY_INTERFACE_BODY( DistributionPiecewiseLinear )
    END_QUERY_INTERFACE_BODY( DistributionPiecewiseLinear )


    // ---------- DistributionFunction::CONSTANT_DISTRIBUTION -------
    DistributionConstant::DistributionConstant( )
        : DistributionBase() 
    {
        type = DistributionFunction::CONSTANT_DISTRIBUTION;
    }

    DistributionConstant::~DistributionConstant()
    {
    }

    float DistributionConstant::Calculate( RANDOMBASE* pRNG ) const
    {
        return m_Param1;
    }

    IDistribution* DistributionConstant::Clone() const
    {
        return new DistributionConstant(*this);
    }

    REGISTER_SERIALIZABLE( DistributionConstant );
    void DistributionConstant::serialize( IArchive& ar, DistributionConstant* obj )
    {
        DistributionBase::serialize( ar, obj );
    }



    //---------------- DistributionFunction::EXPONENTIAL_DISTRIBUTION  -------------------
    DistributionExponential::DistributionExponential()
        : DistributionBase()
    {
        type = DistributionFunction::EXPONENTIAL_DISTRIBUTION;
    }

    DistributionExponential::~DistributionExponential()
    {
    }

    void DistributionExponential::SetParameters(double param1, double param2, double param3)
    {
        DistributionBase::SetParameters( param1, param2, param3 );
    }

    float DistributionExponential::Calculate( RANDOMBASE* pRNG ) const
    {
        return pRNG->expdist( m_Param1 );
    }

    IDistribution* DistributionExponential::Clone() const
    {
        return new DistributionExponential(*this);
    }

    REGISTER_SERIALIZABLE(DistributionExponential);
    void DistributionExponential::serialize( IArchive& ar, DistributionExponential* obj )
    {
        DistributionBase::serialize( ar, obj );
    }



    //---------------- DistributionFunction::GAUSSIAN_DISTRIBUTION  -------------------
    DistributionGaussian::DistributionGaussian()
        : DistributionBase()
    {
        type = DistributionFunction::GAUSSIAN_DISTRIBUTION;
    }

    DistributionGaussian::~DistributionGaussian()
    {
    }

    float DistributionGaussian::Calculate( RANDOMBASE* pRNG ) const
    {
        double value = pRNG->eGauss() * m_Param2 + m_Param1;
        if ( value < 0.0 ) { value = 0.0; }
        return value;
    }

    IDistribution* DistributionGaussian::Clone() const
    {
        return new DistributionGaussian(*this);
    }

    REGISTER_SERIALIZABLE(DistributionGaussian);
    void DistributionGaussian::serialize(IArchive& ar, DistributionGaussian* obj)
    {
        DistributionBase::serialize(ar, obj);
    }


    //---------------- DistributionFunction::POISSON_DISTRIBUTION  -------------------
    DistributionPoisson::DistributionPoisson()
        : DistributionBase()
    {
        type = DistributionFunction::POISSON_DISTRIBUTION;
    }

    DistributionPoisson::~DistributionPoisson()
    {
    }

    float DistributionPoisson::Calculate( RANDOMBASE* pRNG ) const
    {
        return pRNG->Poisson( m_Param1 );
    }

    IDistribution* DistributionPoisson::Clone() const
    {
        return new DistributionPoisson(*this);
    }

    REGISTER_SERIALIZABLE( DistributionPoisson );
    void DistributionPoisson::serialize( IArchive& ar, DistributionPoisson* obj )
    {
        DistributionBase::serialize( ar, obj );
    }


    //---------------- DistributionLogNormal  -------------------
    DistributionLogNormal::DistributionLogNormal()
        : DistributionBase()
    {
        type = DistributionFunction::LOG_NORMAL_DISTRIBUTION;
    }

    DistributionLogNormal::~DistributionLogNormal()
    {
    }

    float DistributionLogNormal::Calculate( RANDOMBASE* pRNG ) const
    {
        return exp( m_Param1 + pRNG->eGauss() * m_Param2 );
    }

    IDistribution* DistributionLogNormal::Clone() const
    {
        return new DistributionLogNormal(*this);
    }

    REGISTER_SERIALIZABLE(DistributionLogNormal);
    void DistributionLogNormal::serialize(IArchive& ar, DistributionLogNormal* obj)
    {
        DistributionBase::serialize(ar, obj);
    }


    //---------------- DistributionFunction::WEIBULL  -------------------
    DistributionWeibull::DistributionWeibull()
        : DistributionBase()
    {
        type = DistributionFunction::WEIBULL_DISTRIBUTION;
    }

    DistributionWeibull::~DistributionWeibull()
    {
    }

    float DistributionWeibull::Calculate( RANDOMBASE* pRNG ) const
    {
        return pRNG->Weibull( m_Param1, m_Param2 );
    }

    IDistribution* DistributionWeibull::Clone() const
    {
        return new DistributionWeibull(*this);
    }

    REGISTER_SERIALIZABLE(DistributionWeibull);
    void DistributionWeibull::serialize(IArchive& ar, DistributionWeibull* obj)
    {
        DistributionBase::serialize(ar, obj);
    }


    //---------------- DistributionFunction::DUAL_CONSTANT_DISTRIBUTION  -------------------
    DistributionDualConstant::DistributionDualConstant()
        : DistributionBase()
    {
        type = DistributionFunction::DUAL_CONSTANT_DISTRIBUTION;
    }

    DistributionDualConstant::~DistributionDualConstant()
    {
    }

    float DistributionDualConstant::Calculate( RANDOMBASE* pRNG ) const
    {
        return ( pRNG->e() < m_Param1 ) ? m_Param2 : 1;
    }


    IDistribution* DistributionDualConstant::Clone() const
    {
        return new DistributionDualConstant(*this);
    }

    REGISTER_SERIALIZABLE( DistributionDualConstant );
    void DistributionDualConstant::serialize( IArchive& ar, DistributionDualConstant* obj )
    {
        DistributionBase::serialize( ar, obj );
    }


    //---------------- DistributionFunction::DUAL_TIMESCALE -------------------
    DistributionDualExponential::DistributionDualExponential()
        : DistributionBase()
    {
        type = DistributionFunction::DUAL_EXPONENTIAL_DISTRIBUTION;
    }

    DistributionDualExponential::~DistributionDualExponential()
    {
    }

    float DistributionDualExponential::Calculate( RANDOMBASE* pRNG ) const
    {
        if ( ( m_Param3 == 1.0 ) || ( ( m_Param3 > 0.0 ) && ( m_Param3 >= pRNG->e() ) ) )
        {
            return pRNG->expdist( m_Param1 );
        }
        else
        {
            return pRNG->expdist( m_Param2 );
        }
    }

    IDistribution* DistributionDualExponential::Clone() const
    {
        return new DistributionDualExponential(*this);
    }

    REGISTER_SERIALIZABLE(DistributionDualExponential);
    void DistributionDualExponential::serialize(IArchive& ar, DistributionDualExponential* obj)
    {
        DistributionBase::serialize(ar, obj);
    }


    //---------------- DistributionFunction::UNIFORM  -------------------
    DistributionUniform::DistributionUniform()
        : DistributionBase()
    {
        type = DistributionFunction::UNIFORM_DISTRIBUTION;
    }

    DistributionUniform::~DistributionUniform()
    {
    }


    float DistributionUniform::Calculate( RANDOMBASE* pRNG ) const
    {
        return m_Param1 + pRNG->e() * ( m_Param2 - m_Param1 );
    }

    IDistribution* DistributionUniform::Clone() const
    {
        return new DistributionUniform(*this);
    }

    REGISTER_SERIALIZABLE(DistributionUniform);
    void DistributionUniform::serialize(IArchive& ar, DistributionUniform* obj)
    {
        DistributionBase::serialize(ar, obj);
    }

    //---------------- DistributionFunction::PIECEWISE_CONSTANT  -------------------
    DistributionPiecewiseConstant::DistributionPiecewiseConstant()
        : PiecewiseDistributionBase()
    {
        //type = DistributionFunction::PIECEWISE_CONSTANT;
    }

    DistributionPiecewiseConstant::~DistributionPiecewiseConstant()
    {
    }

    float DistributionPiecewiseConstant::Calculate( RANDOMBASE* pRNG ) const
    {
        return m_interpolatedValueMap.getValuePiecewiseConstant( m_x );
    }

    IDistribution* DistributionPiecewiseConstant::Clone() const
    {
        return new DistributionPiecewiseConstant(*this);
    }

    REGISTER_SERIALIZABLE( DistributionPiecewiseConstant );
    void DistributionPiecewiseConstant::serialize( IArchive& ar, DistributionPiecewiseConstant* obj )
    {
        PiecewiseDistributionBase::serialize( ar, obj );
    }


    //---------------- DistributionFunction::PIECEWISE_LINEAR  -------------------
    DistributionPiecewiseLinear::DistributionPiecewiseLinear()
        : PiecewiseDistributionBase()
    {
        //type = DistributionFunction::PIECEWISE_LINEAR;
    }

    DistributionPiecewiseLinear::~DistributionPiecewiseLinear()
    {
    }

    float DistributionPiecewiseLinear::Calculate( RANDOMBASE* pRNG ) const
    {
        return m_interpolatedValueMap.getValueLinearInterpolation( m_x );
    }

    IDistribution* DistributionPiecewiseLinear::Clone() const
    {
        return new DistributionPiecewiseLinear(*this);
    }

    REGISTER_SERIALIZABLE( DistributionPiecewiseLinear );
    void DistributionPiecewiseLinear::serialize( IArchive& ar, DistributionPiecewiseLinear* obj )
    {
        PiecewiseDistributionBase::serialize( ar, obj );
    }
}
