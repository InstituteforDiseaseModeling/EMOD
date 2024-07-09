
#pragma once
#include "IdmApi.h"
#include "IPairFormationParameters.h"
#include "IRelationship.h"

class Configuration;

namespace Kernel {

    class IDMAPI PairFormationParamsFactory {
    public:
        static IPairFormationParameters* Create( RelationshipType::Enum relType,
                                                 const Configuration* pConfig );
    };
}
