/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        const IPairFormationParameters* parameters;
        map<int, map<int, vector<int> > > eligible_population;      // < Risk --> < Gender --> Eligible Vector > >

        DECLARE_SERIALIZABLE(PairFormationStatsImpl);
#pragma warning( pop )
    };
}