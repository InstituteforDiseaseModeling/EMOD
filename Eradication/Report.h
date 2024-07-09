
#pragma once

#include <map>
#include <time.h>
#include "BaseChannelReport.h"
#include "Properties.h"
#include "BroadcasterObserver.h"
#include "EventTrigger.h"
#include "InterventionName.h"

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

namespace Kernel
{
class Report : public BaseChannelReport, public IIndividualEventObserver
{
    GET_SCHEMA_STATIC_WRAPPER(Report)
public:
    DECLARE_QUERY_INTERFACE()
    IMPLEMENT_NO_REFERENCE_COUNTING()
public:
    static IReport* CreateReport();
    virtual ~Report() { }

    virtual bool Configure( const Configuration* config ) override;

    virtual void Initialize( unsigned int nrmSize ) override;
    virtual void UpdateEventRegistration( float currentTime,
                                          float dt,
                                          std::vector<INodeEventContext*>& rNodeEventContextList,
                                          ISimulationEventContext* pSimEventContext ) override;
    virtual void BeginTimestep() override;
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override { return true ; };
    virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
    virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
    virtual void EndTimestep( float currentTime, float dt ) override;
    virtual void Reduce() override;

    // IIndividualEventObserver
    virtual bool notifyOnEvent( Kernel::IIndividualHumanEventContext *context, 
                                const EventTrigger& trigger ) override;

protected:
    Report();
    Report( const std::string& rReportName );

    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
    virtual void postProcessAccumulatedData() override;

    virtual void AddSEIRChannels();
    virtual void AddSEIRWUnits( std::map<std::string, std::string> &units_map );
    virtual void UpdateSEIRW( const Kernel::IIndividualHuman* individual, float monte_carlo_weight );
    virtual void AccumulateSEIRW();
    virtual void NormalizeSEIRWChannels();

    virtual const char* GetParameterNameForHasInterventions() const;
    virtual const char* GetDescTextForHasInterventions() const;
    virtual const char* GetDependsOnForHasInterventions() const;

    virtual const char* GetParameterNameForHasIP() const;
    virtual const char* GetDescTextForHasIP() const;
    virtual const char* GetDependsOnForHasIP() const;

    virtual const char* GetParameterNameForIncludePregancies() const;
    virtual const char* GetDescTextForIncludePregancies() const;
    virtual const char* GetDependsOnForIncludePregancies() const;

    std::vector<InterventionName> m_InterventionNames;
    std::vector<std::string> m_IPKeyNames;
    std::vector<IPKeyValue> m_IPKeyValues;
    std::vector< EventTrigger > m_EventTriggerList ;
    std::vector<IIndividualEventBroadcaster*> m_BroadcasterList;
    bool m_IsRegistered;

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

    bool m_IncludePregnancies;
    float m_PossibleMothers;
    float m_NewPregnancies;
    float m_Pregnant;

    ChannelID stat_pop_id;
    ChannelID new_infections_id;
    ChannelID susceptible_pop_id;
    ChannelID exposed_pop_id;
    ChannelID infectious_pop_id;
    ChannelID recovered_pop_id;
    ChannelID waning_pop_id;
    ChannelID hum_infectious_res_id;
    ChannelID log_prev_id;
    ChannelID births_id;
    ChannelID infected_id;
    ChannelID symtomatic_pop_id;
    ChannelID newly_symptomatic_id;
    ChannelID air_temp_id;
    ChannelID land_temp_id;
    ChannelID rainfall_id;
    ChannelID relative_humidity_id;
    ChannelID campaign_cost_id;
    ChannelID disease_deaths_id;
    ChannelID possible_mothers_id;
    ChannelID new_pregnancies_id;
    ChannelID pregnant_id;

    std::vector<ChannelID> m_InterventionNameChannelIDs;
    std::vector<ChannelID> m_IPKeyValuesChannelIDs;
};
}