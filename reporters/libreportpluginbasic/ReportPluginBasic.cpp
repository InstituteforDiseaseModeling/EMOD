/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReportPluginBasic.h"
#include <functional>
#include <map>
#include "BoostLibWrapper.h"
#include "Report.h"
#include "Sugar.h"
#include "Environment.h"
#include "IIndividualHuman.h"
#include "Climate.h"
#include "Interventions.h"

#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"
#include "FactorySupport.h" // for DTK_DLLEXPORT

using namespace std;
using namespace json;

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportPluginBasic" ) // <<< Name of this file

// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "*", nullptr }; // <<< Types of simulation the report is to be used with

// Output file name
static const std::string _report_name = "CustomReport.json"; // <<< Filename to put data into

Kernel::report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new CustomReport()); // <<< Report to create
};

Kernel::DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ------------------------------
// --- DLL Interface Methods
// ---
// --- The DTK will use these methods to establish communication with the DLL.
// ------------------------------

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

DTK_DLLEXPORT char* __cdecl
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
}

DTK_DLLEXPORT void __cdecl
GetSupportedSimTypes(char* simTypes[])
{
    DLL_HELPER.GetSupportedSimTypes( simTypes );
}

DTK_DLLEXPORT const char * __cdecl
GetType()
{
    return DLL_HELPER.GetType();
}

DTK_DLLEXPORT void __cdecl
GetReportInstantiator( Kernel::report_instantiator_function_t* pif )
{
    DLL_HELPER.GetReportInstantiator( pif );
}

#ifdef __cplusplus
}
#endif


// ----------------------------------------
// --- CustomReport Methods
// ----------------------------------------

static const char * _stat_pop_label = "Statistical Population";
static const char * _births_label = "Births";
static const char * _infected_label = "Infected";
static const char * _infected_with_interventions_label = "Infected with Intervention(#)";
static const char * _infected_without_interventions_label = "Infected without Intervention(#)";
static const char * _air_temp_label = "Air Temperature";
static const char * _land_temp_label = "Land Temperature";
static const char * _hum_inf_res_label = "Human Infectious Reservoir";
static const char * _rainfall_label = "Rainfall";
static const char * _rel_humid_label = "Relative Humidity";
static const char * _new_infections_label = "New Infections";
static const char * _cum_infections_label = "Cumulative Infections";
static const char * _new_rep_infs_label = "New Reported Infections";
static const char * _cum_rep_infs_label = "Cumulative Reported Infections";
static const char * _disease_deaths_label = "Disease Deaths";
static const char * _camp_cost_label = "Campaign Cost";
static const char * _inf_rate_label = "Daily (Human) Infection Rate";
static const char * _log_prev_label = "Log Prevalence";


/////////////////////////
// Initialization methods
/////////////////////////


CustomReport::CustomReport()
{
    LOG_INFO( "CustomReport ctor\n" );
    report_name = _report_name;
}

/////////////////////////
// steady-state methods
/////////////////////////
void
CustomReport::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    LOG_DEBUG( "LogNodeData.\n" );

    Accumulate(_stat_pop_label, pNC->GetStatPop() );
    Accumulate(_births_label,   pNC->GetBirths()  );
    Accumulate(_infected_label, pNC->GetInfected()); 

    //if (pNode->GetFlags()->climate_structure != CLIMATE_OFF)
    if (pNC->GetLocalWeather())
    {
        //Accumulate(_air_temp_label,  pNC->GetLocalWeather()->airtemperature());
        //Accumulate(_land_temp_label, pNC->GetLocalWeather()->landtemperature());
        Accumulate(_rainfall_label,  pNC->GetLocalWeather()->accumulated_rainfall());
        Accumulate(_rel_humid_label, pNC->GetLocalWeather()->humidity());
    }

    //Accumulate(_new_infections_label, pNC->GetNewInfections());
    //Accumulate(_cum_infections_label, pNC->GetCumulativeInfections());
    //Accumulate(_new_rep_infs_label,   pNC->GetNewReportedInfections());
    //Accumulate(_cum_rep_infs_label,   pNC->GetCumulativeReportedInfections());
    //Accumulate(_disease_deaths_label, pNC->GetDiseaseDeaths());
    Accumulate(_camp_cost_label,      pNC->GetCampaignCost());
    Accumulate(_hum_inf_res_label,    pNC->GetInfectivity());
    Accumulate(_inf_rate_label,       pNC->GetInfectionRate());
}

void 
CustomReport::LogIndividualData(
    Kernel::IIndividualHuman* individual
)
{
    LOG_DEBUG( "LogIndividualData\n" );
    float infectedWithIntervention = 0.0f;
    float infectedWithoutIntervention = 0.0f;
    // get monte carlo weight
    float mc_weight = (float)individual->GetMonteCarloWeight();
    // figure out if individual is infected
    if (individual->IsInfected())
    {
        // figure out if individual has an intervention
        Kernel::IIndividualHumanInterventionsContext * intervs = individual->GetInterventionsContext();
        //std::ostringstream oss;
        //oss << "num intervs = " << intervs->size() << std::endl;
        //LOG_INFO( oss.str() );
        const std::list<Kernel::IDistributableIntervention*> &intervsByTypeList = intervs->GetInterventionsByType( "class Kernel::SimpleVaccine" );
        if( intervsByTypeList.size() )
        {
            //LOG_INFO( "Found individual who received vaccine but got infected.\n" );
            infectedWithIntervention += mc_weight;
        }
        else
        {
            infectedWithoutIntervention += mc_weight;
        }
    }
    Accumulate( _infected_with_interventions_label, infectedWithIntervention );
    Accumulate( _infected_without_interventions_label, infectedWithoutIntervention );
}

// Just calling the base class but for demo purposes leaving in because I can imagine wanting to do this custom.
void
CustomReport::Finalize()
{
    LOG_INFO( "WriteData\n" );
    return BaseChannelReport::Finalize();
}

void 
CustomReport::populateSummaryDataUnitsMap(
    std::map<std::string, std::string> &units_map
)
{
    LOG_INFO( "populateSummaryDataUnitsMap\n" );
    units_map[_stat_pop_label]                       = "Population";
    units_map[_births_label]                         = _births_label;
    units_map[_infected_label]                       = "Infected fraction";
    units_map[_infected_with_interventions_label]    = "Individuals";
    units_map[_infected_without_interventions_label] = "Individuals";
    units_map[_log_prev_label]                       = _log_prev_label;
    units_map[_rainfall_label]                       = "mm/day";
    //units_map["Temperature"]                         = "degrees C";
    units_map[_new_infections_label]                 = "";
    units_map[_cum_infections_label]                 = "";
    units_map[_new_rep_infs_label]                   = "";
    units_map[_cum_rep_infs_label]                   = "";
    //units_map["New Disease Deaths"]                  = "";
    //units_map["Cumulative Disease Deaths"]           = "";
    units_map[_camp_cost_label]                      = "USD";
    units_map[_hum_inf_res_label]                    = "Total Infectivity";
    units_map[_inf_rate_label]                       = "Infection Rate";
}

// not sure whether to leave this in custom demo subclass
void
CustomReport::postProcessAccumulatedData()
{
    LOG_DEBUG( "getSummaryDataCustomProcessed\n" );
    normalizeChannel(_infected_label,                     _stat_pop_label);
    //normalizeChannel(channel_name, &value, _air_temp_label,              timestep, (float)nrmSize);
    //normalizeChannel(channel_name, &value, _land_temp_label,             timestep, (float)nrmSize);
    normalizeChannel(_hum_inf_res_label,  (float)_nrmSize);
    normalizeChannel(_inf_rate_label,     (float)_nrmSize);
    normalizeChannel(_rainfall_label,     (float)_nrmSize * (1 / 1000.0f)); // multiply by 1000 to get result in mm/day
}
