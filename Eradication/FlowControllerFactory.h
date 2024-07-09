
#pragma once

#include "IdmApi.h"

namespace Kernel
{
    struct IPairFormationAgent;
    struct IPairFormationRateTable;
    struct IPairFormationStats;
    struct IPairFormationParameters;
    struct IPairFormationFlowController;

    class FlowControllerFactory {
    public:
        IDMAPI static IPairFormationFlowController* CreateController(
            IPairFormationAgent*,
            IPairFormationStats*,
            IPairFormationRateTable*,
            const IPairFormationParameters*);
    };
}