
#pragma once

#include "IdmApi.h"
#include "IPairFormationAgent.h"
#include "IPairFormationParameters.h"
#include "IRelationship.h"

namespace Kernel {

    class IDMAPI PfaFactory {
    public:
        static IPairFormationAgent* CreatePfa( const Configuration* pConfig,
                                               const IPairFormationParameters* params,
                                               float selectionThreshold,
                                               RANDOMBASE* prng, 
                                               RelationshipCreator rc );
    };
}