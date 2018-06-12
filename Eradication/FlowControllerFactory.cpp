/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
