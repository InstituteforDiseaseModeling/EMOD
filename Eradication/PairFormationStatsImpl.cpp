/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "PairFormationStatsImpl.h"
#include "SimulationEnums.h" // MALE/FEMALE

#include "Log.h"
#include "Debug.h"

SETUP_LOGGING( "PairFormationStatsImpl" )

namespace Kernel 
{
    BEGIN_QUERY_INTERFACE_BODY(PairFormationStatsImpl)
    END_QUERY_INTERFACE_BODY(PairFormationStatsImpl)

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
        if( parameters != nullptr )
        {
            for( int risk_group = 0; risk_group < RiskGroup::Enum::COUNT; risk_group++ )    // TODO: Need better way to iterate through an enum
            {
                eligible_population[risk_group][Gender::MALE  ].resize(parameters->GetMaleAgeBinCount());
                eligible_population[risk_group][Gender::FEMALE].resize(parameters->GetFemaleAgeBinCount());
            }
        }
    }

    PairFormationStatsImpl::~PairFormationStatsImpl()
    {
        // Nothing to do here (yet?)
    }

    void PairFormationStatsImpl::SetParameters( const IPairFormationParameters* params )
    {
        parameters = params;
        if( parameters != nullptr )
        {
            release_assert( eligible_population.size() == RiskGroup::COUNT );
            for( int i = 0 ; i < RiskGroup::COUNT ; i++ )
            {
                release_assert( eligible_population[i].size() == Gender::COUNT );
                release_assert( eligible_population[i][Gender::MALE  ].size() == parameters->GetMaleAgeBinCount()   );
                release_assert( eligible_population[i][Gender::FEMALE].size() == parameters->GetFemaleAgeBinCount() );
            }
        }
    }

    REGISTER_SERIALIZABLE(PairFormationStatsImpl);

    void PairFormationStatsImpl::serialize(IArchive& ar, PairFormationStatsImpl* obj)
    {
        PairFormationStatsImpl& stats = *obj;
        ar.labelElement("eligible_population") & stats.eligible_population;

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Needs to be set during serialization
        //const IPairFormationParameters* parameters;
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
}