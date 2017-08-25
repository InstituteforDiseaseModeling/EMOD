/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <functional>
#include <map>

#include "Report.h"
#include "Sugar.h"
#include "Environment.h"
#include "INodeContext.h"
#include "IIndividualHuman.h"
#include "Climate.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "Report" )

const string Report::_stat_pop_label       ( "Statistical Population" );
const string Report::_susceptible_pop_label( "Susceptible Population" );
const string Report::_exposed_pop_label    ( "Exposed Population" );
const string Report::_infectious_pop_label ( "Infectious Population" );
const string Report::_recovered_pop_label  ( "Recovered Population" );
const string Report::_waning_pop_label     ( "Waning Population" );
const string Report::_immunized_pop_label  ( "Immunized Population" );

static const std::string _report_name        ( "InsetChart.json" );
const string Report::_new_infections_label   ( "New Infections" );
const string Report::_infected_fraction_label( "Infected Fraction" );
const string Report::_new_reported_infections_label( "New Reported Infections" );
const string Report::_cum_reported_infections_label( "Cumulative Reported Infections" );
const string Report::_hum_infectious_res_label( "Human Infectious Reservoir" );
const string Report::_log_prev_label( "Log Prevalence" );
const string Report::_infection_rate_label( "Daily (Human) Infection Rate" );
//const string Report::_aoi_label( "Mean Age Of Infection" );

/////////////////////////
// Initialization methods
/////////////////////////
Kernel::IReport*
Report::CreateReport()
{
    return new Report();
}

Report::Report()
: last_time(0)
{
    report_name = _report_name;

    disease_deaths = 0.0f;
}


/////////////////////////
// steady-state methods
/////////////////////////
void Report::BeginTimestep()
{
    BaseChannelReport::BeginTimestep();

    new_infections = 0.0f;
    new_reported_infections = 0.0f;

    // SEIRW reporting
    countOfSusceptibles = 0.0f;
    countOfExposed      = 0.0f;
    countOfInfectious   = 0.0f;
    countOfRecovered    = 0.0f;
    countOfWaning       = 0.0f;
}

void Report::EndTimestep( float currentTime, float dt )
{
    Accumulate("Disease Deaths", disease_deaths);
    BaseChannelReport::EndTimestep( currentTime, dt );
}

void
Report::LogIndividualData(
    Kernel::IIndividualHuman* individual
)
{
    float monte_carlo_weight = float(individual->GetMonteCarloWeight());

    NewInfectionState::_enum nis = individual->GetNewInfectionState();

    if(nis == NewInfectionState::NewAndDetected || nis == NewInfectionState::NewInfection)
        new_infections += monte_carlo_weight;

    if(nis == NewInfectionState::NewAndDetected || nis == NewInfectionState::NewlyDetected)
        new_reported_infections += monte_carlo_weight;

    if(individual->GetStateChange() == HumanStateChange::KilledByInfection)
        disease_deaths += monte_carlo_weight;

    UpdateSEIRW(individual, monte_carlo_weight);
}

void
Report::LogNodeData(
    Kernel::INodeContext * pNC
)
{

    LOG_DEBUG( "LogNodeData\n" );

    Accumulate(_stat_pop_label, pNC->GetStatPop());
    Accumulate("Births", pNC->GetBirths());
    Accumulate("Infected", pNC->GetInfected());
    //Accumulate(_aoi_label: pNC->GetMeanAgeInfection() ); // * pNC->GetInfected());

    if (pNC->GetLocalWeather())
    {
        Accumulate("Air Temperature",   pNC->GetLocalWeather()->airtemperature());
        Accumulate("Land Temperature",  pNC->GetLocalWeather()->landtemperature());
        Accumulate("Rainfall",          pNC->GetLocalWeather()->accumulated_rainfall());
        Accumulate("Relative Humidity", pNC->GetLocalWeather()->humidity());
    }

    Accumulate(_new_infections_label,                 new_infections);
    new_infections = 0.0f;
    Accumulate(_new_reported_infections_label,        new_reported_infections);
    new_reported_infections = 0.0f;

    Accumulate("Campaign Cost",                  pNC->GetCampaignCost());
    Accumulate("Human Infectious Reservoir",     pNC->GetInfectivity());
    Accumulate(_infection_rate_label,   pNC->GetInfectionRate());

    AccumulateSEIRW();
}

void 
Report::populateSummaryDataUnitsMap(
    std::map<std::string, std::string> &units_map
)
{
    units_map[_stat_pop_label]                  = "Population";
    units_map["Births"]                         = "Births";
    units_map["Infected"]                       = _infected_fraction_label;
    units_map[_log_prev_label]                  = "Log Prevalence";
    units_map["Rainfall"]                       = "mm/day";
    units_map["Temperature"]                    = "degrees C";
    units_map[_new_infections_label]            = "";
    units_map["Cumulative Infections"]          = "";
    units_map["Reported New Infections"]        = "";
    units_map[_cum_reported_infections_label]   = "";
    units_map["Disease Deaths"]                 = "";
    units_map["Campaign Cost"]                  = "USD";
    units_map[_hum_infectious_res_label]        = "Total Infectivity";
    units_map[_infection_rate_label]            = "Infection Rate";

    AddSEIRWUnits(units_map);
}


// normalize by timestep and create derived channels
void
Report::postProcessAccumulatedData()
{
    LOG_DEBUG( "postProcessAccumulatedData\n" );

    normalizeChannel("Infected", _stat_pop_label);
    if( channelDataMap.HasChannel( "Air Temperature" ) )
    {
        normalizeChannel("Air Temperature", float(_nrmSize));
        normalizeChannel("Land Temperature", float(_nrmSize));
        normalizeChannel("Relative Humidity", float(_nrmSize));
        normalizeChannel("Rainfall", float(_nrmSize) * (1 / 1000.0f)); // multiply by 1000 to get result in mm/day
    }
    normalizeChannel( _hum_infectious_res_label, float(_nrmSize) );
    normalizeChannel( _infection_rate_label, float(_nrmSize) );

    // add derived channels
    addDerivedLogScaleSummaryChannel("Infected", _log_prev_label);
    addDerivedCumulativeSummaryChannel(_new_infections_label, "Cumulative Infections");
    addDerivedCumulativeSummaryChannel(_new_reported_infections_label, _cum_reported_infections_label);

    NormalizeSEIRWChannels();
}

void Report::UpdateSEIRW( const Kernel::IIndividualHuman* individual, float monte_carlo_weight )
{
    if (!individual->IsInfected())  // Susceptible, Recovered (Immune), or Waning
    {
        float acquisitionModifier = individual->GetImmunityReducedAcquire() * individual->GetInterventionReducedAcquire();
        if (acquisitionModifier >= 1.0f)
        {
            countOfSusceptibles += monte_carlo_weight;
        }
        else if (acquisitionModifier > 0.0f)
        {
            countOfWaning += monte_carlo_weight;
        }
        else
        {
            countOfRecovered += monte_carlo_weight;
        }
    }
    else // Exposed or Infectious 
    {
        if (individual->GetInfectiousness() > 0.0f)
        {
            countOfInfectious += monte_carlo_weight;
        }
        else
        {
            countOfExposed += monte_carlo_weight;
        }
    }
}

void Report::AccumulateSEIRW()
{
    Accumulate(_susceptible_pop_label, countOfSusceptibles);
    Accumulate(_exposed_pop_label,     countOfExposed);
    Accumulate(_infectious_pop_label,  countOfInfectious);
    Accumulate(_recovered_pop_label,   countOfRecovered);
    Accumulate(_waning_pop_label,      countOfWaning);

    countOfSusceptibles = 0.0f;
    countOfExposed      = 0.0f;
    countOfInfectious   = 0.0f;
    countOfRecovered    = 0.0f;
    countOfWaning       = 0.0f;
}

void Report::AddSEIRWUnits( std::map<std::string, std::string> &units_map )
{
    units_map[_susceptible_pop_label] = "Susceptible Fraction";
    units_map[_exposed_pop_label]     = "Exposed Fraction";
    units_map[_infectious_pop_label]  = "Infectious Fraction";
    units_map[_recovered_pop_label]   = "Recovered (Immune) Fraction";
    units_map[_waning_pop_label]      = "Waning Immunity Fraction";
}

void Report::NormalizeSEIRWChannels()
{
    normalizeChannel(_susceptible_pop_label, _stat_pop_label);
    normalizeChannel(_exposed_pop_label,     _stat_pop_label);
    normalizeChannel(_infectious_pop_label,  _stat_pop_label);
    normalizeChannel(_recovered_pop_label,   _stat_pop_label);
    normalizeChannel(_waning_pop_label,      _stat_pop_label);
}

#if 0
template<class Archive>
void serialize(Archive &ar, Report& report, const unsigned int v)
{
    boost::serialization::void_cast_register<Report,IReport>();
    ar & report.timesteps_reduced;
    ar & report.channelDataMap;
    ar & report._nrmSize;
}
#endif
