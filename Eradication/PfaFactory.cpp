
#include "stdafx.h"
#include "PfaFactory.h"
#include "BehaviorPfa.h"

namespace Kernel {

    IPairFormationAgent* PfaFactory::CreatePfa( const Configuration* pConfig,
                                                const IPairFormationParameters* params,
                                                float selectionThreshold,
                                                RANDOMBASE* prng, 
                                                RelationshipCreator rc )
    {
        IPairFormationAgent* pfa = BehaviorPfa::CreatePfa( pConfig, params, selectionThreshold, prng, rc );
        return pfa;
    }
}