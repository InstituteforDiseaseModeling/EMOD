
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
        virtual std::map<int, std::vector<float>>& GetDesiredFlow();

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

        IPairFormationAgent* pair_formation_agent;
        IPairFormationStats* pair_formation_stats;
        IPairFormationRateTable* rate_table;
        const IPairFormationParameters* parameters;

        float rate_ratio[Gender::COUNT];

        std::map<int, std::vector<float>> desired_flow;

        DECLARE_SERIALIZABLE(FlowControllerImpl);
    };
}
