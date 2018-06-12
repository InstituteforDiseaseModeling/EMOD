/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "RateTableImpl.h"
#include "Common.h"
#include "SimulationEnums.h"
#include "Debug.h"
#include "Log.h"

SETUP_LOGGING( "RateTableImpl" )

namespace Kernel 
{
    BEGIN_QUERY_INTERFACE_BODY(RateTableImpl)
    END_QUERY_INTERFACE_BODY(RateTableImpl)

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

    RateTableImpl::RateTableImpl( const IPairFormationParameters* params )
        : rate_table()
        , parameters(params)
    {
        if( parameters != nullptr )
        {
            for( int risk_group = 0; risk_group < RiskGroup::COUNT; risk_group++ )
            {
                rate_table[risk_group][Gender::MALE  ].resize(parameters->GetMaleAgeBinCount(),   0.0f);
                rate_table[risk_group][Gender::FEMALE].resize(parameters->GetFemaleAgeBinCount(), 0.0f);
            }
        }
    }

    RateTableImpl::~RateTableImpl()
    {
        // Nothing to do at the moment.
    }

    void RateTableImpl::SetParameters( const IPairFormationParameters* params )
    {
        parameters = params;
        if( parameters != nullptr )
        {
            release_assert( rate_table.size() == RiskGroup::COUNT );
            for( int i = 0 ; i < RiskGroup::COUNT ; i++ )
            {
                release_assert( rate_table[i].size() == Gender::COUNT );
                release_assert( rate_table[i][Gender::MALE  ].size() == parameters->GetMaleAgeBinCount()   );
                release_assert( rate_table[i][Gender::FEMALE].size() == parameters->GetFemaleAgeBinCount() );
            }
        }
    }

    REGISTER_SERIALIZABLE(RateTableImpl);

    void RateTableImpl::serialize(IArchive& ar, RateTableImpl* obj)
    {
        RateTableImpl& table = *obj;
        ar.labelElement("rate_table") & table.rate_table;

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Needs to be set during serialization
        //const IPairFormationParameters* parameters;
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
}