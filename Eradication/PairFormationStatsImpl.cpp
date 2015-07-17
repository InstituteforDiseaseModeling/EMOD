/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "PairFormationStatsImpl.h"
#include "SimulationEnums.h" // MALE/FEMALE

#include "Log.h"
#include "Debug.h"
static const char * _module = "PairFormationStatsImpl";

namespace Kernel {

    void PairFormationStatsImpl::ResetEligible()
    {
        for (int risk_group = 0; risk_group < RiskGroup::COUNT; risk_group++)  // Risk Group
        {
            for (auto& entry : eligible_population.at(risk_group))
            {
                vector<int>& population = entry.second;
                memset( population.data(), 0, population.size() * sizeof(int) );
            }
        }
    }

    void PairFormationStatsImpl::UpdateEligible( float age_in_days, int sex, RiskGroup::Enum risk_group, int delta )
    {
        LOG_DEBUG_F("%s( %f, %d, %s, %d )\n", __FUNCTION__, age_in_days, sex, RiskGroup::pairs::lookup_key(risk_group), delta);
        int agebin_index = parameters->BinIndexForAgeAndSex(age_in_days, sex);
        release_assert( (eligible_population.at(risk_group).at(sex)[agebin_index] + delta) >= 0 );
        eligible_population.at(risk_group).at(sex)[agebin_index] += delta;
    }

    const map<int, vector<int>>& PairFormationStatsImpl::GetEligible(RiskGroup::Enum risk_group)
    {
        return eligible_population.at(risk_group);
    }

    IPairFormationStats* PairFormationStatsImpl::CreateStats(const IPairFormationParameters* params)
    {
        IPairFormationStats* pfs = _new_ PairFormationStatsImpl(params);
        return pfs;
    }

    PairFormationStatsImpl::PairFormationStatsImpl(const IPairFormationParameters* params)
        : parameters(params)
        , eligible_population()
    {
        for( int risk_group = 0; risk_group < RiskGroup::Enum::COUNT; risk_group++ )    // TODO: Need better way to iterate through an enum
        {
            eligible_population[risk_group][Gender::MALE].resize(parameters->GetMaleAgeBinCount());
            eligible_population[risk_group][Gender::FEMALE].resize(parameters->GetFemaleAgeBinCount());
        }
    }

    PairFormationStatsImpl::~PairFormationStatsImpl()
    {
        // Nothing to do here (yet?)
    }
}