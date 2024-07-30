
#pragma once
#include "IdmApi.h"
#include "IPairFormationStats.h"
#include "IPairFormationParameters.h"

namespace Kernel {

    class PairFormationStatsFactory {
    public:
        IDMAPI static IPairFormationStats* CreateStatistician(const IPairFormationParameters*);
    };
}
