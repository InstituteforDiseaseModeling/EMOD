/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "IPairFormationFlowController.h"
#include "SimulationEnums.h"

#include <map>
#include <vector>

namespace Kernel 
{

    struct IPairFormationAgent;
    struct IPairFormationStats;
    struct IPairFormationRateTable;
    struct IPairFormationParameters;

    class IDMAPI FlowControllerImpl : public IPairFormationFlowController 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();
    public:

        virtual void UpdateEntryRates( const IdmDateTime& rCurrentTime, float dt );

        static IPairFormationFlowController* CreateController(
            IPairFormationAgent*,
            IPairFormationStats*,
            IPairFormationRateTable*,
            const IPairFormationParameters*);

    protected:
        FlowControllerImpl( IPairFormationAgent* agent=nullptr,
                            IPairFormationStats* stats=nullptr, 
                            IPairFormationRateTable* table=nullptr,
                            const IPairFormationParameters* params=nullptr );
        virtual ~FlowControllerImpl();

        void UpdateDesiredFlow( const IdmDateTime& rCurrentTime, float dt );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        IPairFormationAgent* pair_formation_agent;
        IPairFormationStats* pair_formation_stats;
        IPairFormationRateTable* rate_table;
        const IPairFormationParameters* parameters;

        float rate_ratio[Gender::COUNT];

        std::map<int, std::vector<float>> desired_flow;

        DECLARE_SERIALIZABLE(FlowControllerImpl);
#pragma warning( pop )
    };
}
