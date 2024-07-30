
#include "stdafx.h"
#include "FlowControllerFactory.h"
#include "FlowControllerImpl.h"

namespace Kernel {

    IPairFormationFlowController* Kernel::FlowControllerFactory::CreateController(
        IPairFormationAgent* pair_formation_agent,
        IPairFormationStats* pair_formation_stats,
        IPairFormationRateTable* rate_table,
        const IPairFormationParameters* parameters)
    {
        IPairFormationFlowController* pfc = FlowControllerImpl::CreateController(
            pair_formation_agent,
            pair_formation_stats,
            rate_table,
            parameters);

        return pfc;
    }
};
