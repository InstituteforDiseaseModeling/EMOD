/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "FlowControllerImpl.h"
#include "IPairFormationStats.h"
#include "IPairFormationRateTable.h"
#include "IPairFormationParameters.h"

#include "Log.h"

SETUP_LOGGING( "FlowControllerImpl" )

namespace Kernel 
{
    BEGIN_QUERY_INTERFACE_BODY(FlowControllerImpl)
    END_QUERY_INTERFACE_BODY(FlowControllerImpl)

    void FlowControllerImpl::UpdateEntryRates( const IdmDateTime& rCurrentTime, float dt )
    {
        LOG_DEBUG_F("%s()\n", __FUNCTION__);

        // -------------------------
        // --- Update desired rates
        // -------------------------
        UpdateDesiredFlow( rCurrentTime, dt );

        if (LOG_LEVEL(INFO))
        {
            LOG_INFO_F( "%s: desired flow:\n", __FUNCTION__ );
            for (auto& entry : desired_flow) {
                cout << "{ " << entry.first << ", [ ";
                for (float flow : entry.second) {
                    cout << flow << ' ';
                }
                cout << "] }" << endl;
            }
        }

        // --------------------------------------------------------------
        // --- Get the desired rate(s) from the pfa,
        // --- Normalize by the number of eligible from the stats object
        // --- Update the desired rates in the rate table
        // --------------------------------------------------------------
        auto& eligible_population_LOW  = pair_formation_stats->GetEligible(RiskGroup::LOW);
        auto& eligible_population_HIGH = pair_formation_stats->GetEligible(RiskGroup::HIGH);

        for (int sex = Gender::MALE; sex <= Gender::FEMALE; sex++)
        {
            auto& desired = desired_flow.at(sex);
            auto& eligible_LOW  = eligible_population_LOW.at(sex);
            auto& eligible_HIGH = eligible_population_HIGH.at(sex);
            for (int bin_index = 0; bin_index < desired_flow[sex].size(); bin_index++)
            {
                int eligible_count_LOW = eligible_LOW[bin_index];
                int eligible_count_HIGH = eligible_HIGH[bin_index];
                int effective_eligible_count = rate_ratio[sex] * eligible_count_HIGH + eligible_count_LOW;
                
                float rate_LOW = (effective_eligible_count > 0) ? desired[bin_index] / effective_eligible_count : 0.0f;
                float rate_HIGH = rate_ratio[sex] * rate_LOW;
                
                rate_table->SetRateForBinAndSexAndRiskGroup(bin_index, sex, RiskGroup::LOW, rate_LOW);
                rate_table->SetRateForBinAndSexAndRiskGroup(bin_index, sex, RiskGroup::HIGH, rate_HIGH);
            }
        }

        if (LOG_LEVEL(INFO))
        {
            rate_table->DumpRates();
        }
    }

    IPairFormationFlowController* FlowControllerImpl::CreateController(
        IPairFormationAgent* pair_formation_agent,
        IPairFormationStats* pair_formation_stats,
        IPairFormationRateTable* rate_table,
        const IPairFormationParameters* parameters)
    {
        IPairFormationFlowController* pfc = _new_ FlowControllerImpl(pair_formation_agent, pair_formation_stats, rate_table, parameters);
        return pfc;
    }

    FlowControllerImpl::FlowControllerImpl(IPairFormationAgent* pfa, IPairFormationStats* stats, IPairFormationRateTable* rates, const IPairFormationParameters* params)
        : pair_formation_agent(pfa)
        , pair_formation_stats(stats)
        , rate_table(rates)
        , parameters(params)
        , desired_flow()
    {
        if( parameters != nullptr )
        {
            rate_ratio[Gender::MALE  ] = parameters->GetRateRatio(Gender::MALE);
            rate_ratio[Gender::FEMALE] = parameters->GetRateRatio(Gender::FEMALE);

            desired_flow[Gender::MALE  ].resize(parameters->GetMaleAgeBinCount());
            desired_flow[Gender::FEMALE].resize(parameters->GetFemaleAgeBinCount());
        }
    }

    FlowControllerImpl::~FlowControllerImpl()
    {
        pair_formation_agent = nullptr;
        pair_formation_stats = nullptr;
        rate_table = nullptr;
        parameters = nullptr;
    }

    void FlowControllerImpl::UpdateDesiredFlow( const IdmDateTime& rCurrentTime, float dt )
    {
        LOG_DEBUG_F("%s()\n", __FUNCTION__);

        // ----------------------------------------------------------
        // --- Count the total number of people eligible for pairing
        // ----------------------------------------------------------
        float cumulative_base_flow = 0.0f;
        
        for( int risk_group = 0; risk_group < RiskGroup::COUNT; risk_group ++)
        {
            auto& eligible_population = pair_formation_stats->GetEligible((RiskGroup::Enum)risk_group);
            for (auto& entry : eligible_population)
            {
                for (float flow : entry.second)
                {
                    cumulative_base_flow += flow;
                }
            }

            if (LOG_LEVEL(INFO))
            {
                LOG_INFO_F( "%s: eligible population for %s risk group:\n", __FUNCTION__, RiskGroup::pairs::lookup_key(risk_group) );
                for (auto& entry : eligible_population) {
                    cout << "{ " << entry.first << ", [ ";
                    for (int count : entry.second) {
                        cout << count << ' ';
                    }
                    cout << "] }" << endl;
                }
            }
        }

        // ---------------------------------------------------------------------
        // --- Multiply the total number of people eligible times the base rate
        // ---------------------------------------------------------------------
        cumulative_base_flow *= parameters->FormationRate( rCurrentTime, dt );

        if (cumulative_base_flow > 0.0f)
        {
            // -----------------------------------------------------------
            // --- Determine the desired rate for each gender and age bin
            // -----------------------------------------------------------
            for (int sex = Gender::MALE; sex <= Gender::FEMALE; sex++)
            {
                auto& desired = desired_flow.at(sex);       // important, use a reference here so we update desired_flow
                auto& marginal = parameters->MarginalValues().at(sex);   // use a reference here to avoid a copy
                int bin_count = desired.size();
                for (int bin_index = 0; bin_index < bin_count; bin_index++)
                {
                    desired[bin_index] = 0.5f * cumulative_base_flow * marginal[bin_index];
                }
            }
        }
        else
        {
            memset(desired_flow[Gender::MALE].data(), 0, desired_flow[Gender::MALE].size() * sizeof(float));
            memset(desired_flow[Gender::FEMALE].data(), 0, desired_flow[Gender::FEMALE].size() * sizeof(float));
        }
    }
    REGISTER_SERIALIZABLE(FlowControllerImpl);

    void FlowControllerImpl::serialize(IArchive& ar, FlowControllerImpl* obj)
    {
        FlowControllerImpl& flow = *obj;
        ar.labelElement("rate_ratio"              ); ar.serialize( flow.rate_ratio, Gender::COUNT );
        ar.labelElement("desired_flow"            ) & flow.desired_flow;

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Needs to be set during serialization
        //IPairFormationAgent* pair_formation_agent;
        //IPairFormationStats* pair_formation_stats;
        //IPairFormationRateTable* rate_table;
        //const IPairFormationParameters* parameters;
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
}
