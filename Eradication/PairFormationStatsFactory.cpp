
#include "stdafx.h"
#include "PairFormationStatsFactory.h"
#include "PairFormationStatsImpl.h"

namespace Kernel {

    IPairFormationStats* PairFormationStatsFactory::CreateStatistician(const IPairFormationParameters* parameters)
    {
        IPairFormationStats* pfs = PairFormationStatsImpl::CreateStats(parameters);
        return pfs;
    }
}
