/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "IdmApi.h"
#include "IPairFormationFlowController.h"
#include "IPairFormationRateTable.h"
#include "IPairFormationStats.h"
#include "IPairFormationParameters.h"

namespace Kernel {

    class FlowControllerFactory {
    public:
        IDMAPI static IPairFormationFlowController* CreateController(
            IPairFormationAgent*,
            IPairFormationStats*,
            IPairFormationRateTable*,
            const IPairFormationParameters*);
    };
}