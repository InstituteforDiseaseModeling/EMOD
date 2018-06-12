/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include <time.h>
#include "BaseChannelReport.h"

/*
Flexible aggregator for simulation-wide numeric values. Intended to serve as the mechanism for reporting inset charts.
The class interface and usage pattern is customized for computing, for each timestep, a sum (accumulate) of a labeled quantity 
across the simulation.

Accumulate values into the sum for the current timestep with a call to Accumulate("ChannelName", value).
Wrap each timestep's accumulation events in calls to Begin/EndTimestep().
All the channels that will ever be accumulated into need to have had Accumulate() called for them before the first EndTimestep(),
or unspecified behavior may result later.

To finalize the accumulation across distributed MPI processes, call Reduce(); data will be valid on rank 0 only.
*/

class Report : public BaseChannelReport
{
public:
    static IReport* CreateReport();
    virtual ~Report() { }

    virtual void BeginTimestep() override;
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override { return true ; };
    virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
    virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
    virtual void EndTimestep( float currentTime, float dt ) override;

protected:
    Report();
    Report( const std::string& rReportName );

    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
    virtual void postProcessAccumulatedData() override;

    virtual void AddSEIRWUnits( std::map<std::string, std::string> &units_map );
    virtual void UpdateSEIRW( const Kernel::IIndividualHuman* individual, float monte_carlo_weight );
    virtual void AccumulateSEIRW();
    virtual void NormalizeSEIRWChannels();

    // counters
    float new_infections;
    float new_reported_infections;
    float disease_deaths;

    float countOfSusceptibles;
    float countOfExposed;
    float countOfInfectious;
    float countOfRecovered;
    float countOfImmunized;
    float countOfWaning;
    clock_t last_time;

    static const std::string Report::_stat_pop_label;
    static const std::string Report::_infected_fraction_label;
    static const std::string Report::_new_infections_label;

    static const std::string Report::_susceptible_pop_label;
    static const std::string Report::_exposed_pop_label;
    static const std::string Report::_infectious_pop_label;
    static const std::string Report::_recovered_pop_label;
    static const std::string Report::_waning_pop_label;
    static const std::string Report::_immunized_pop_label;
    static const std::string Report::_new_reported_infections_label;
    static const std::string Report::_cum_reported_infections_label;
    static const std::string Report::_hum_infectious_res_label;
    static const std::string Report::_log_prev_label;
    static const std::string Report::_infection_rate_label;
    //static const std::string Report::_aoi_label;
};
