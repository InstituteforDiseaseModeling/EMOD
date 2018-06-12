/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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