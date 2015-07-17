/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <list>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <time.h>

#include "Log.h"
#include "BaseChannelReport.h"
#include "BoostLibWrapper.h"

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

    virtual void BeginTimestep();
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const { return true ; } ;
    virtual void LogIndividualData( Kernel::IndividualHuman * individual );
    virtual void LogNodeData( Kernel::INodeContext * pNC );
    virtual void EndTimestep( float currentTime, float dt );

protected:
    Report();

    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );
    virtual void postProcessAccumulatedData();

    virtual void AddSEIRWUnits( std::map<std::string, std::string> &units_map );
    virtual void UpdateSEIRW( const Kernel::IndividualHuman * individual, float monte_carlo_weight );
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
    static const std::string Report::_new_reported_infections_label;
    static const std::string Report::_cum_reported_infections_label;
    static const std::string Report::_hum_infectious_res_label;
    static const std::string Report::_log_prev_label;
    static const std::string Report::_prob_new_infection_label;

private:
#if USE_BOOST_SERIALIZATION
    friend class ::boost::serialization::access;
    template<class Archive>
    friend void serialize(Archive &ar, Report& report, const unsigned int v);
#endif
};
