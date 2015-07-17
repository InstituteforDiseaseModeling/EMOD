/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "IPairFormationFlowController.h"
#include "IPairFormationStats.h"
#include "IPairFormationRateTable.h"
#include "IPairFormationParameters.h"

#include <map>
#include <vector>

namespace Kernel {

    class IDMAPI FlowControllerImpl : public IPairFormationFlowController {
    public:

        virtual void UpdateEntryRates();

        static IPairFormationFlowController* CreateController(
            IPairFormationAgent*,
            IPairFormationStats*,
            IPairFormationRateTable*,
            const IPairFormationParameters*);

    protected:
        FlowControllerImpl(IPairFormationAgent*, IPairFormationStats*, IPairFormationRateTable*, const IPairFormationParameters*);
        virtual ~FlowControllerImpl();

        void UpdateDesiredFlow();

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        IPairFormationAgent* pair_formation_agent;
        IPairFormationStats* pair_formation_stats;
        IPairFormationRateTable* rate_table;
        const IPairFormationParameters* parameters;

        float rate_ratio[Gender::COUNT];

        map<int, vector<float>> desired_flow;

        const map<int, vector<float>>& marginal_values;

        float base_pair_formation_rate;
#pragma warning( pop )
    };
}
