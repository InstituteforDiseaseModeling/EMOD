
#pragma once
#include "IdmApi.h"
#include "IPairFormationRateTable.h"
#include "IPairFormationParameters.h"
#include <map>
#include <vector>

using namespace std;

namespace Kernel {

    class IDMAPI RateTableImpl : public IPairFormationRateTable 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();
    public:
        virtual float GetRateForAgeAndSexAndRiskGroup(float age, int sex, RiskGroup::Enum risk_group) const override;
        virtual void SetRateForBinAndSexAndRiskGroup(int bin_index, int sex, RiskGroup::Enum risk_group, float value) override;
        virtual void SetParameters( const IPairFormationParameters* params ) override;
        virtual void DumpRates() override;

        static IPairFormationRateTable* CreateRateTable(const IPairFormationParameters*);

    protected:

        RateTableImpl(const IPairFormationParameters* params = nullptr);
        ~RateTableImpl();

        typedef map<int, map<int, vector<float> > > RateTable_t;

        RateTable_t rate_table;
        const IPairFormationParameters* parameters;

        DECLARE_SERIALIZABLE(RateTableImpl);
    };
}