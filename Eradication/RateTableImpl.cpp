/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "RateTableImpl.h"
#include "Common.h"
#include "SimulationEnums.h"

static const char * _module = "RateTableImpl";

namespace Kernel {

    float RateTableImpl::GetRateForAgeAndSexAndRiskGroup( float age, int sex, RiskGroup::Enum risk_group ) const
    {
        int bin = parameters->BinIndexForAgeAndSex(age, sex);
        return rate_table.at(risk_group).at(sex)[bin];
    }

    void RateTableImpl::SetRateForBinAndSexAndRiskGroup( int bin, int sex, RiskGroup::Enum risk_group, float value )
    {
        rate_table.at(risk_group).at(sex)[bin] = value;
    }

    void RateTableImpl::DumpRates()
    {
        for( int risk_group = 0; risk_group < RiskGroup::COUNT; risk_group++ )
        {
            cout << '[' << _module << "] " << __FUNCTION__ << ": Current rates for risk group " << RiskGroup::pairs::lookup_key(risk_group) << " -" << endl;
            for (auto& entry : rate_table.at(risk_group)) {
                cout << "{ " << entry.first << ", [ " ;
                for (float rate : entry.second) {
                    cout << rate << ' ';
                }
                cout << "] }" << endl;
            }
        }
    }

    IPairFormationRateTable* RateTableImpl::CreateRateTable(const IPairFormationParameters* params)
    {
        IPairFormationRateTable* prt = _new_ RateTableImpl(params);
        return prt;
    }

    RateTableImpl::RateTableImpl(const IPairFormationParameters* params)
        : rate_table()
        , parameters(params)
    {
        for( int risk_group = 0; risk_group < RiskGroup::COUNT; risk_group++ )
        {
            rate_table[risk_group][Gender::MALE].resize(parameters->GetMaleAgeBinCount(), 0.0f);
            rate_table[risk_group][Gender::FEMALE].resize(parameters->GetFemaleAgeBinCount(), 0.0f);
        }
    }

    RateTableImpl::~RateTableImpl()
    {
        // Nothing to do at the moment.
    }
}