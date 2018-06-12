/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        typedef map<int, map<int, vector<float> > > RateTable_t;

        RateTable_t rate_table;
        const IPairFormationParameters* parameters;

        DECLARE_SERIALIZABLE(RateTableImpl);
#pragma warning( pop )
    };
}