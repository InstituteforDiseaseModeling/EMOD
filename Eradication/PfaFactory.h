/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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