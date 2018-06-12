/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "IdmApi.h"
#include "IPairFormationParameters.h"
#include "IRelationship.h"

class Configuration;

namespace Kernel {

    class IDMAPI PairFormationParamsFactory {
    public:
        static const IPairFormationParameters* Create( RelationshipType::Enum relType,
                                                       const Configuration* pConfig );
    };
}
