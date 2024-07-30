
#include "stdafx.h"
#include "MathFunctions.h"
#include "DistributionFactory.h"
#include "Distributions.h"
#include "DistributionsConfigurable.h"
#include "IDistribution.h"

SETUP_LOGGING( "DistributionFactory" )

namespace Kernel
{
    IDistribution* DistributionFactory::CreateDistribution( DistributionFunction::Enum infectious_distribution_function )
    {
        switch( infectious_distribution_function )
        {
        case DistributionFunction::CONSTANT_DISTRIBUTION:
            return new DistributionConstant();
        case DistributionFunction::GAUSSIAN_DISTRIBUTION:
            return new DistributionGaussian();
        case DistributionFunction::POISSON_DISTRIBUTION:
            return new DistributionPoisson();
        case DistributionFunction::EXPONENTIAL_DISTRIBUTION:
            return new DistributionExponential();
        case DistributionFunction::LOG_NORMAL_DISTRIBUTION:
            return new DistributionLogNormal();
        case DistributionFunction::DUAL_EXPONENTIAL_DISTRIBUTION:
            return new DistributionDualExponential();
        case DistributionFunction::WEIBULL_DISTRIBUTION:
            return new DistributionWeibull();
        case DistributionFunction::DUAL_CONSTANT_DISTRIBUTION:
            return new DistributionDualConstant();
        case DistributionFunction::UNIFORM_DISTRIBUTION:
            return new DistributionUniform();
        case DistributionFunction::NOT_INITIALIZED:
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Distribution is NOT_INITIALIZED" );
        default:
            throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "DistributionFunction does not exist." );
        }
    }

    IDistribution* DistributionFactory::CreateDistribution( JsonConfigurable* pParent,
                                                            DistributionFunction::Enum distribution_function,
                                                            const std::string& base_parameter_name,
                                                            const Configuration* config )
    {
        if( JsonConfigurable::_dryrun )
        {
            AddToSchema( pParent, distribution_function, base_parameter_name, config );
            return nullptr;
        }

        switch( distribution_function )
        {
            case DistributionFunction::CONSTANT_DISTRIBUTION:
            {
                DistributionConstantConfigurable* distribution = new DistributionConstantConfigurable();
                distribution->Configure( pParent, base_parameter_name, config );
                return distribution;
            }
            case DistributionFunction::GAUSSIAN_DISTRIBUTION:
            {
                DistributionGaussianConfigurable* distribution = new DistributionGaussianConfigurable();
                distribution->Configure( pParent, base_parameter_name, config );
                return distribution;
            }
            case DistributionFunction::POISSON_DISTRIBUTION:
            {
                DistributionPoissonConfigurable* distribution = new DistributionPoissonConfigurable();
                distribution->Configure( pParent, base_parameter_name, config );
                return distribution;
            }
            case DistributionFunction::EXPONENTIAL_DISTRIBUTION:
            {
                DistributionExponentialConfigurable* distribution = new DistributionExponentialConfigurable();
                distribution->Configure( pParent, base_parameter_name, config );
                return distribution;
            }
            case DistributionFunction::LOG_NORMAL_DISTRIBUTION:
            {
                DistributionLogNormalConfigurable* distribution = new DistributionLogNormalConfigurable();
                distribution->Configure( pParent, base_parameter_name, config );
                return distribution;
            }
            case DistributionFunction::DUAL_EXPONENTIAL_DISTRIBUTION:
            {
                DistributionDualExponentialConfigurable* distribution = new DistributionDualExponentialConfigurable();
                distribution->Configure( pParent, base_parameter_name, config );
                return distribution;
            }
            case DistributionFunction::WEIBULL_DISTRIBUTION:
            {
                DistributionWeibullConfigurable* distribution = new DistributionWeibullConfigurable();
                distribution->Configure( pParent, base_parameter_name, config );
                return distribution;
            }
            case DistributionFunction::DUAL_CONSTANT_DISTRIBUTION:
            {
                DistributionDualConstantConfigurable* distribution = new DistributionDualConstantConfigurable();
                distribution->Configure( pParent, base_parameter_name, config );
                return distribution;
            }
            case DistributionFunction::UNIFORM_DISTRIBUTION:
            {
                DistributionUniformConfigurable* distribution = new DistributionUniformConfigurable();
                distribution->Configure( pParent, base_parameter_name, config );
                return distribution;
            }
            case DistributionFunction::NOT_INITIALIZED:
            {
                std::stringstream ss;
                ss << "The distribution '" << base_parameter_name+"_Distribution" << "' is NOT_INITIALIZED in class " << pParent->GetTypeName();
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            default:
                throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "DistributionFunction does not exist." );
        }
    }

    void DistributionFactory::AddToSchema( JsonConfigurable* pParent,
                                           DistributionFunction::Enum distribution_function,
                                           const std::string& base_parameter_name,
                                           const Configuration* config )
    {
        DistributionConstantConfigurable distribution_constant;
        distribution_constant.Configure( pParent, base_parameter_name, config );

        DistributionGaussianConfigurable distribution_gaussian;
        distribution_gaussian.Configure( pParent, base_parameter_name, config );

        DistributionPoissonConfigurable distribution_poisson;
        distribution_poisson.Configure( pParent, base_parameter_name, config );

        DistributionExponentialConfigurable distribution_exponential;
        distribution_exponential.Configure( pParent, base_parameter_name, config );

        DistributionLogNormalConfigurable distribution_log_normal;
        distribution_log_normal.Configure( pParent, base_parameter_name, config );

		DistributionDualExponentialConfigurable distribution_dual_exponential;
		distribution_dual_exponential.Configure(pParent, base_parameter_name, config);

        DistributionWeibullConfigurable distribution_weibull;
        distribution_weibull.Configure( pParent, base_parameter_name, config );

        DistributionDualConstantConfigurable distribution_dual_constant;
        distribution_dual_constant.Configure( pParent, base_parameter_name, config );

        DistributionUniformConfigurable distribution_uniform;
        distribution_uniform.Configure( pParent, base_parameter_name, config );
    }
}
