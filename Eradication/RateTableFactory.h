
#pragma once
#include "IdmApi.h"
#include "IPairFormationRateTable.h"
#include "IPairFormationParameters.h"

using namespace std;

namespace Kernel {

    class RateTableFactory
    {
    public:
        IDMAPI static IPairFormationRateTable* CreateRateTable(const IPairFormationParameters*);
    };
}
