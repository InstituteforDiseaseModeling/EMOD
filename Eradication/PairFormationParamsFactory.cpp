/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "PairFormationParamsFactory.h"
#include "PairFormationParametersImpl.h"

namespace Kernel {

    const IPairFormationParameters* PairFormationParamsFactory::Create( RelationshipType::Enum relType,
                                                                        const Configuration* pConfig, 
                                                                        float base_rate, 
                                                                        float rate_ratio_male, 
                                                                        float rate_ratio_female )
    {
        return PairFormationParametersImpl::CreateParameters( relType, pConfig, base_rate, rate_ratio_male, rate_ratio_female );
    }
}
