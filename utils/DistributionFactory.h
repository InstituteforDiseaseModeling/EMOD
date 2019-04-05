/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Configure.h"
#include "IDistribution.h"


namespace Kernel
{
    class DistributionFactory
    {
    public:
        static IDistribution* CreateDistribution( DistributionFunction::Enum infectious_distribution_function );
        static IDistribution* CreateDistribution( JsonConfigurable * pParent, DistributionFunction::Enum infectious_distribution_function, std::string base_parameter_name, const Configuration * config );
        static IDistribution* CreateDistribution( DistributionFunction::Enum infectious_distribution_function, std::string base_parameter_name, const Configuration* config );
    private:
        static void AddToSchema( JsonConfigurable* pParent, DistributionFunction::Enum infectious_distribution_function, std::string base_parameter_name, const Configuration* config );
    };
}