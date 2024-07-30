
#include "stdafx.h"
#include "DistributionsConfigurable.h"
#include "IArchive.h"
#include "RANDOM.h"
#include "Environment.h"
#include "PiecewiseDistributionBase.h"
#include "InterpolatedValueMap.h"

SETUP_LOGGING("DistributionsConfigurable")

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( DistributionExponentialConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionExponentialConfigurable )

    BEGIN_QUERY_INTERFACE_BODY( DistributionGaussianConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionGaussianConfigurable )

    BEGIN_QUERY_INTERFACE_BODY( DistributionPoissonConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionPoissonConfigurable )

    BEGIN_QUERY_INTERFACE_BODY( DistributionLogNormalConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionLogNormalConfigurable )

    BEGIN_QUERY_INTERFACE_BODY( DistributionConstantConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionConstantConfigurable )

    BEGIN_QUERY_INTERFACE_BODY( DistributionWeibullConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionWeibullConfigurable )

    BEGIN_QUERY_INTERFACE_BODY( DistributionDualConstantConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionBimodaConfigurablel )

    BEGIN_QUERY_INTERFACE_BODY( DistributionDualExponentialConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionDualExponentialConfigurable )

    BEGIN_QUERY_INTERFACE_BODY( DistributionUniformConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionUniformConfigurable )

    BEGIN_QUERY_INTERFACE_BODY( DistributionPiecewiseConstantConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionPiecewiseConstantConfigurable )

    BEGIN_QUERY_INTERFACE_BODY( DistributionPiecewiseLinearConfigurable )
    END_QUERY_INTERFACE_BODY( DistributionPiecewiseLinearConfigurable )
    

    
    // ---------- DistributionFunctionConfigurable::CONSTANT_DISTRIBUTION -------
    //GET_SCHEMA_STATIC_WRAPPER_IMPL( DistributionConstantConfigurable, DistributionConstantConfigurable )
    DistributionConstantConfigurable::DistributionConstantConfigurable()
        : DistributionConstant()
    {
        m_Param1 = 6.0;
    }

    DistributionConstantConfigurable::~DistributionConstantConfigurable()
    { }

    bool DistributionConstantConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_constant( param_name + "_Constant" );
        const std::string distribution_name( param_name + "_Distribution" );

        pParent->initConfigTypeMap( param_constant.c_str(), &m_Param1, Distribution_Constant_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, distribution_name.c_str(), "CONSTANT_DISTRIBUTION" );
        return pParent->JsonConfigurable::Configure( config );
    }

    IDistribution* DistributionConstantConfigurable::Clone() const
    {
        return new DistributionConstantConfigurable( *this );
    }

    REGISTER_SERIALIZABLE( DistributionConstantConfigurable );
    void DistributionConstantConfigurable::serialize( IArchive& ar, DistributionConstantConfigurable* obj )
    {
        DistributionConstant::serialize( ar, obj );
    }



    //---------------- DistributionFunctionConfigurable::EXPONENTIAL_DISTRIBUTION  -------------------
    DistributionExponentialConfigurable::DistributionExponentialConfigurable( )
        : DistributionExponential()
    {
        m_Param1 = 6.0;
    }

    DistributionExponentialConfigurable::~DistributionExponentialConfigurable()
    { }

    bool DistributionExponentialConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_exponential( param_name + "_Exponential" );
        const std::string distribution_name( param_name + "_Distribution" );

        pParent->initConfigTypeMap( param_exponential.c_str(), &m_Param1, Distribution_Exponential_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, distribution_name.c_str(), "EXPONENTIAL_DISTRIBUTION" );
        bool ret = pParent->JsonConfigurable::Configure( config );

        if( !JsonConfigurable::_dryrun && ret )
        {
	        m_Param1 = ( double )1.0 / (double)m_Param1; //convert parameter to a rate
        }
        return ret;
    }

    IDistribution* DistributionExponentialConfigurable::Clone() const
    {
        return new DistributionExponentialConfigurable( *this );
    }

    REGISTER_SERIALIZABLE( DistributionExponentialConfigurable );
    void DistributionExponentialConfigurable::serialize( IArchive& ar, DistributionExponentialConfigurable* obj )
    {
        DistributionExponential::serialize( ar, obj );
    }



    //---------------- DistributionFunctionConfigurable::GAUSSIAN_DISTRIBUTION  -------------------
    DistributionGaussianConfigurable::DistributionGaussianConfigurable( )
        : DistributionGaussian()
    {
        m_Param1 = 6.0;
        m_Param2 = 1.0;
    }

    DistributionGaussianConfigurable::~DistributionGaussianConfigurable()
    { }

    bool DistributionGaussianConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_gaussian_mean( param_name + "_Gaussian_Mean" );
        const std::string param_gaussian_std_dev( param_name + "_Gaussian_Std_Dev" );
        const std::string distribution_name( param_name + "_Distribution" );

        pParent->initConfigTypeMap( param_gaussian_mean.c_str(), &m_Param1, Distribution_Gaussian_Mean_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, distribution_name.c_str(), "GAUSSIAN_DISTRIBUTION" );
        pParent->initConfigTypeMap( param_gaussian_std_dev.c_str(), &m_Param2, Distribution_Gaussian_Std_Dev_DESC_TEXT, FLT_MIN, FLT_MAX, 1.0f, distribution_name.c_str(), "GAUSSIAN_DISTRIBUTION" );
        return pParent->JsonConfigurable::Configure( config );
    }

    IDistribution* DistributionGaussianConfigurable::Clone() const
    {
        return new DistributionGaussianConfigurable( *this );
    }

    REGISTER_SERIALIZABLE(DistributionGaussianConfigurable );
    void DistributionGaussianConfigurable::serialize(IArchive& ar, DistributionGaussianConfigurable* obj)
    {
        DistributionGaussian::serialize(ar, obj);
    }


    //---------------- DistributionFunctionConfigurable::POISSON_DISTRIBUTION  -------------------
    DistributionPoissonConfigurable::DistributionPoissonConfigurable( )
        : DistributionPoisson()
    {
        m_Param1 = 6.0;
    }

    DistributionPoissonConfigurable::~DistributionPoissonConfigurable()
    { }

    bool DistributionPoissonConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_poisson_mean( param_name + "_Poisson_Mean" );
        const std::string distribution_name( param_name + "_Distribution" );

        pParent->initConfigTypeMap( param_poisson_mean.c_str(), &m_Param1, Distribution_Poisson_Mean_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, distribution_name.c_str(), "POISSON_DISTRIBUTION" );
        return pParent->JsonConfigurable::Configure( config );
    }

    IDistribution* DistributionPoissonConfigurable::Clone() const
    {
        return new DistributionPoissonConfigurable( *this );
    }

    REGISTER_SERIALIZABLE( DistributionPoissonConfigurable );
    void DistributionPoissonConfigurable::serialize( IArchive& ar, DistributionPoissonConfigurable* obj )
    {
        DistributionPoisson::serialize( ar, obj );
    }


    //---------------- DistributionLogNormal  -------------------
    DistributionLogNormalConfigurable::DistributionLogNormalConfigurable( )
        : DistributionLogNormal()
    {
        m_Param1 = 6.0;
        m_Param2 = 1.0;
    }


    DistributionLogNormalConfigurable::~DistributionLogNormalConfigurable()
    { }

    bool DistributionLogNormalConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_log_normal_mu( param_name + "_Log_Normal_Mu" );
        const std::string param_log_normal_sigma( param_name + "_Log_Normal_Sigma" );
        const std::string distribution_name( param_name + "_Distribution" );

        pParent->initConfigTypeMap( param_log_normal_mu.c_str(), &m_Param1, Distribution_LogNormal_Mu_DESC_TEXT, -FLT_MAX, FLT_MAX, 6.0f, distribution_name.c_str(), "LOG_NORMAL_DISTRIBUTION" );
        pParent->initConfigTypeMap( param_log_normal_sigma.c_str(), &m_Param2, Distribution_LogNormal_Sigma_DESC_TEXT, -FLT_MAX, FLT_MAX, 1.0f, distribution_name.c_str(), "LOG_NORMAL_DISTRIBUTION" );
        return pParent->JsonConfigurable::Configure( config );
    }

    IDistribution* DistributionLogNormalConfigurable::Clone() const
    {
        return new DistributionLogNormalConfigurable( *this );
    }

    REGISTER_SERIALIZABLE( DistributionLogNormalConfigurable );
    void DistributionLogNormalConfigurable::serialize(IArchive& ar, DistributionLogNormalConfigurable* obj)
    {
        DistributionLogNormal::serialize(ar, obj);
    }



    //---------------- DistributionFunctionConfigurable::WEIBULL  -------------------
    DistributionWeibullConfigurable::DistributionWeibullConfigurable( )
        : DistributionWeibull()
    {
        m_Param1 = 1.0;
        m_Param2 = 1.0;
    }


    DistributionWeibullConfigurable::~DistributionWeibullConfigurable()
    {
    }

    bool DistributionWeibullConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_lambda( param_name + "_Lambda" );
        const std::string param_kappa( param_name + "_Kappa" );
        const std::string distribution_name( param_name + "_Distribution" );

        pParent->initConfigTypeMap( param_lambda.c_str(), &m_Param1, Distribution_Weibull_Lambda_DESC_TEXT, FLT_MIN, FLT_MAX, 1.0f, distribution_name.c_str(), "WEIBULL_DISTRIBUTION" );
        pParent->initConfigTypeMap( param_kappa.c_str(), &m_Param2, Distribution_Weibull_Kappa_DESC_TEXT, FLT_MIN, FLT_MAX, 1.0f, distribution_name.c_str(), "WEIBULL_DISTRIBUTION" );

        return pParent->JsonConfigurable::Configure( config );
    }

    IDistribution* DistributionWeibullConfigurable::Clone() const
    {
        return new DistributionWeibullConfigurable( *this );
    }

    REGISTER_SERIALIZABLE( DistributionWeibullConfigurable );
    void DistributionWeibullConfigurable::serialize(IArchive& ar, DistributionWeibullConfigurable* obj)
    {
        DistributionWeibull::serialize(ar, obj);
    }


    //---------------- DistributionFunctionConfigurable::DUAL_CONSTANT_DISTRIBUTION  -------------------
    DistributionDualConstantConfigurable::DistributionDualConstantConfigurable( )
        : DistributionDualConstant()
    {
        m_Param1 = 1.0;
        m_Param2 = 1.0;
    }

    DistributionDualConstantConfigurable::~DistributionDualConstantConfigurable()
    {
    }

    bool DistributionDualConstantConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_proportion_0( param_name + "_Proportion_0" );
        const std::string param_peak_2_value( param_name + "_Peak_2_Value" );
        const std::string distribution_name( param_name + "_Distribution" );

        pParent->initConfigTypeMap( param_proportion_0.c_str(), &m_Param1, Distribution_Dual_Constant_Proportion_0_DESC_TEXT, 0.0f, 1.0f, 1.0f, distribution_name.c_str(), "DUAL_CONSTANT_DISTRIBUTION" );
        pParent->initConfigTypeMap( param_peak_2_value.c_str(), &m_Param2, Distribution_Dual_Constant_Peak_2_Value_DESC_TEXT, 0, FLT_MAX, 1.0f, distribution_name.c_str(), "DUAL_CONSTANT_DISTRIBUTION" );

        bool ret = pParent->JsonConfigurable::Configure( config );

        if (( m_Param1 < 0 || m_Param1 > 1 || m_Param2 < 0 ))
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Parameters for Bimodal distribution are not valid." );
        }

        return ret;
    }

    IDistribution* DistributionDualConstantConfigurable::Clone() const
    {
        return new DistributionDualConstantConfigurable( *this );
    }

    REGISTER_SERIALIZABLE( DistributionDualConstantConfigurable );
    void DistributionDualConstantConfigurable::serialize( IArchive& ar, DistributionDualConstantConfigurable* obj )
    {
        DistributionDualConstant::serialize( ar, obj );
    }


    //---------------- DistributionFunctionConfigurable::DUAL_EXPONENTIAL_DISTRIBUTION -------------------
    DistributionDualExponentialConfigurable::DistributionDualExponentialConfigurable( )
        : DistributionDualExponential()
    {
        m_Param1 = 1.0;
        m_Param2 = 1.0;
        m_Param3 = 1.0;
    }

    DistributionDualExponentialConfigurable::~DistributionDualExponentialConfigurable()
    { }

    bool DistributionDualExponentialConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_mean_1( param_name + "_Mean_1" );
        const std::string param_mean_2( param_name + "_Mean_2" );
        const std::string param_proportion_1( param_name + "_Proportion_1" );
        const std::string distribution_name( param_name + "_Distribution" );

        pParent->initConfigTypeMap( param_mean_1.c_str(), &m_Param1, Distribution_DualExponential_Mean_1_DESC_TEXT, FLT_MIN, FLT_MAX, 1.0f, distribution_name.c_str(), "DUAL_EXPONENTIAL_DISTRIBUTION" );
        pParent->initConfigTypeMap( param_mean_2.c_str(), &m_Param2, Distribution_DualExponential_Mean_2_DESC_TEXT, FLT_MIN, FLT_MAX, 1.0f, distribution_name.c_str(), "DUAL_EXPONENTIAL_DISTRIBUTION" );
        pParent->initConfigTypeMap( param_proportion_1.c_str(), &m_Param3, Distribution_DualExponential_Proportion_1_DESC_TEXT, 0.0f, 1.0f, 1.0f, distribution_name.c_str(), "DUAL_EXPONENTIAL_DISTRIBUTION" );

        bool configured = pParent->JsonConfigurable::Configure( config );

        if( configured && !JsonConfigurable::_dryrun )
        {
	        m_Param1 = ( double )1.0 / (double)m_Param1;
	        m_Param2 = ( double )1.0 / (double)m_Param2;
        }
        return configured;
    }

    IDistribution* DistributionDualExponentialConfigurable::Clone() const
    {
        return new DistributionDualExponentialConfigurable( *this );
    }

    REGISTER_SERIALIZABLE( DistributionDualExponentialConfigurable );
    void DistributionDualExponentialConfigurable::serialize(IArchive& ar, DistributionDualExponentialConfigurable* obj)
    {
        DistributionDualExponential::serialize(ar, obj);
    }



    //---------------- DistributionFunctionConfigurable::UNIFORM  -------------------
    DistributionUniformConfigurable::DistributionUniformConfigurable( )
        : DistributionUniform()
    {
        m_Param1 = 0.0;
        m_Param2 = 1.0;
    }

    DistributionUniformConfigurable::~DistributionUniformConfigurable()
    { }

    bool DistributionUniformConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_min( param_name + "_Min" );
        const std::string param_max( param_name + "_Max" );
        const std::string distribution_name( param_name + "_Distribution" );

        pParent->initConfigTypeMap( param_min.c_str(), &m_Param1, Distribution_Uniform_Min_DESC_TEXT, 0.0f, FLT_MAX, 0.0f, distribution_name.c_str(), "UNIFORM_DISTRIBUTION" );
        pParent->initConfigTypeMap( param_max.c_str(), &m_Param2, Distribution_Uniform_Max_DESC_TEXT, 0.0f, FLT_MAX, 1.0f, distribution_name.c_str(), "UNIFORM_DISTRIBUTION" );
        return pParent->JsonConfigurable::Configure( config );
    }  

    IDistribution* DistributionUniformConfigurable::Clone() const
    {
        return new DistributionUniformConfigurable( *this );
    }

    REGISTER_SERIALIZABLE( DistributionUniformConfigurable );
    void DistributionUniformConfigurable::serialize(IArchive& ar, DistributionUniformConfigurable* obj)
    {
        DistributionUniform::serialize(ar, obj);
    }


    //---------------- DistributionFunction::PIECEWISE_CONSTANT  -------------------
    DistributionPiecewiseConstantConfigurable::DistributionPiecewiseConstantConfigurable()
        : DistributionPiecewiseConstant()
    { }

    DistributionPiecewiseConstantConfigurable::~DistributionPiecewiseConstantConfigurable()
    { }

    bool DistributionPiecewiseConstantConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_piecewise_constant( param_name + "_Piecewise_Constant" );
        const std::string distribution_name( param_name + "_Distribution" );
        pParent->initConfigTypeMap( param_piecewise_constant.c_str(), &m_interpolatedValueMap, Distribution_PiecewiseConstant_DESC_TEXT, distribution_name.c_str(), "PIECEWISE_CONSTANT" );
        return pParent->JsonConfigurable::Configure( config );
    }

    IDistribution* DistributionPiecewiseConstantConfigurable::Clone() const
    {
        return new DistributionPiecewiseConstantConfigurable( *this );
    }

    REGISTER_SERIALIZABLE( DistributionPiecewiseConstantConfigurable );
    void DistributionPiecewiseConstantConfigurable::serialize( IArchive& ar, DistributionPiecewiseConstantConfigurable* obj )
    {
        DistributionPiecewiseConstant::serialize( ar, obj );
    }


    //---------------- DistributionFunction::PIECEWISE_LINEAR  -------------------
    DistributionPiecewiseLinearConfigurable::DistributionPiecewiseLinearConfigurable()
        : DistributionPiecewiseLinear()
    { }

    DistributionPiecewiseLinearConfigurable::~DistributionPiecewiseLinearConfigurable()
    { }

    bool DistributionPiecewiseLinearConfigurable::Configure( JsonConfigurable* pParent, const std::string& param_name, const Configuration* config )
    {
        const std::string param_piecewise_linear( param_name + "_Piecewise_Linear" );
        const std::string distribution_name( param_name + "_Distribution" );

        pParent->initConfigTypeMap( param_piecewise_linear.c_str(), &m_interpolatedValueMap, Distribution_PiecewiseLinear_DESC_TEXT, distribution_name.c_str(), "PIECEWISE_LINEAR" );
        return pParent->JsonConfigurable::Configure( config );
    }

    IDistribution* DistributionPiecewiseLinearConfigurable::Clone() const
    {
        return new DistributionPiecewiseLinearConfigurable( *this );
    }

    REGISTER_SERIALIZABLE( DistributionPiecewiseLinearConfigurable );
    void DistributionPiecewiseLinearConfigurable::serialize( IArchive& ar, DistributionPiecewiseLinearConfigurable* obj )
    {
        DistributionPiecewiseLinear::serialize( ar, obj );
    }
   
}
