
#pragma once
#include "IPairFormationStats.h"
#include "IPairFormationParameters.h"

namespace Kernel 
{
    class IDMAPI PairFormationStatsImpl : public IPairFormationStats 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();
    public:
        virtual void ResetEligible() override;
        virtual void UpdateEligible(float age_in_days, int sex, RiskGroup::Enum risk_group, int delta) override;
        virtual const map<int, vector<int>>& GetEligible(RiskGroup::Enum risk_group) override;
        virtual void SetParameters( const IPairFormationParameters* params ) override;

        static IPairFormationStats* CreateStats(const IPairFormationParameters*);

    protected:
        PairFormationStatsImpl( const IPairFormationParameters* params=nullptr);
        virtual ~PairFormationStatsImpl();

        const IPairFormationParameters* parameters;
        map<int, map<int, vector<int> > > eligible_population;      // < Risk --> < Gender --> Eligible Vector > >

        DECLARE_SERIALIZABLE(PairFormationStatsImpl);
    };
}