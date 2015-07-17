/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "IPairFormationStats.h"
#include "IPairFormationParameters.h"

namespace Kernel {

    class IDMAPI PairFormationStatsImpl : public IPairFormationStats {

    public:
        virtual void ResetEligible();
        virtual void UpdateEligible(float age_in_days, int sex, RiskGroup::Enum risk_group, int delta);
        virtual const map<int, vector<int>>& GetEligible(RiskGroup::Enum risk_group);

        static IPairFormationStats* CreateStats(const IPairFormationParameters*);

    protected:
        PairFormationStatsImpl(const IPairFormationParameters*);
        virtual ~PairFormationStatsImpl();

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        const IPairFormationParameters* parameters;
        map<int, map<int, vector<int> > > eligible_population;      // < Risk --> < Gender --> Eligible Vector > >
#pragma warning( pop )
    };
}