
#include "stdafx.h"
#include "Configure.h"
#include "IDistribution.h"


namespace Kernel
{
    class DistributionFactory
    {
    public:
        static IDistribution* CreateDistribution( DistributionFunction::Enum infectious_distribution_function );
        static IDistribution* CreateDistribution( JsonConfigurable * pParent,
                                                  DistributionFunction::Enum infectious_distribution_function,
                                                  const std::string& base_parameter_name,
                                                  const Configuration * config );
        static IDistribution* CreateDistribution( DistributionFunction::Enum infectious_distribution_function,
                                                  const std::string& base_parameter_name,
                                                  const Configuration* config );
    private:
        static void AddToSchema( JsonConfigurable* pParent,
                                 DistributionFunction::Enum infectious_distribution_function,
                                 const std::string& base_parameter_name,
                                 const Configuration* config );
    };
}