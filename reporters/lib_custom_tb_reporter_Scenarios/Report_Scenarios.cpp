/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include <map>

#include "Report_Scenarios.h"
#include "TBContexts.h"
#include "Individual.h"
#include "NodeEventContext.h" //for calling INodeTriggredInterventionConsumer
#include "Debug.h"
#include "TBInterventionsContainer.h"

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

// Module name for logging, CustomReport.json, and DLL GetType()
static const char * _module = "Report_Scenarios"; // <<< Name of this file

namespace Kernel{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "TB_SIM", nullptr };  // <<< Types of simulation the report is to be used with

static const std::string _report_name = "Report_Scenarios.json"; // <<< Filename to put data into

report_instantiator_function_t rif = []()
{
    return (IReport*)(new Report_Scenarios()); // <<< Report to create
};

DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//channels done at the node level
static std::string _stat_pop_label                          = "Statistical Population";
static std::string _prob_new_inf_label                      = "Probability of New Infection";
static std::string _new_active_label                        = "New Active TB Infections";
static std::string _stat_pop_births_label                   = "Births";
static std::string _stat_pop_deaths_label                   = "Deaths";

//channels done at the individual level - done by direct accumulate 
static std::string _disease_deaths_label                    = "Disease Deaths";
static std::string _new_active_mdr_label                    = "New Active TB Infections MDR";
static std::string _new_active_acquired_mdr_label           = "New Active TB Infections Acquired MDR";
static std::string _new_active_fast_label                   = "New Active TB Infections From Fast";
static std::string _new_active_twoyrs_label                 = "New Active TB Infections TwoYrs";
static std::string _latent_TB_label                         = "Latent TB Population";
static std::string _latent_TB_fast_label                    = "Latent TB Fast Population";
static std::string _latent_TB_slow_label                    = "Latent TB Slow Population";
static std::string _latent_ontx_label                       = "Latent OnTreatment";
static std::string _active_TB_label                         = "Active TB Population";
static std::string _active_TB_mdr_label                     = "Active TB Population MDR";
static std::string _active_TB_acquired_mdr_label            = "Active TB Population Acquired MDR";
static std::string _active_TB_naive_label                   = "Active TB Population Naive";
static std::string _active_TB_retx_label                    = "Active TB Population Retx";
static std::string _active_TB_naive_mdr_label               = "Active TB Population Naive MDR";
static std::string _active_TB_retx_mdr_label                = "Active TB Population Retx MDR";
static std::string _active_TB_empirictx_label               = "Active TB Population OnEmpiricTreatment";
static std::string _active_TB_secondlinecombo_label         = "Active TB Population OnSecondLineCombo";
static std::string _secondlinecombo_label                   = "Population OnSecondLineCombo";
static std::string _active_TB_empirictx_mdr_label           = "Active TB Population MDR OnEmpiricTreatment";
static std::string _active_TB_secondlinecombo_mdr_label     = "Active TB Population MDR OnSecondLineCombo";
static std::string _no_active_TB_label                      = "No Active TB Population";
static std::string _active_smearpos_TB_label                = "Active Smearpos TB Population"; 

static std::string _num_tbactivation_label                  = "Num TBActivation";
static std::string _num_providerorderstbtest_label          = "Num ProviderOrdersTBTest";
static std::string _num_tbtestdefault_label                 = "Num TBTestDefault";
static std::string _num_tbtestnegative_label                = "Num TBTestNegative";
static std::string _num_tbtestpositive_label                = "Num TBTestPositive";
static std::string _num_tbmdrtestdefault_label              = "Num TBMDRTestDefault";
static std::string _num_tbmdrtestnegative_label             = "Num TBMDRTestNegative";
static std::string _num_tbmdrtestpositive_label             = "Num TBMDRTestPositive";
static std::string _num_tbstartdrugregimen_label            = "Num TBStartDrugRegimen";
static std::string _num_naive_tbstartdrugregimen_label      = "Num Naive TBStartDrugRegimen";
static std::string _num_retx_tbstartdrugregimen_label       = "Num Retx TBStartDrugRegimen";
static std::string _num_tbfaileddrugregimen_label           = "Num TBFailedDrugRegimen";


//define your age bins
static const int _num_age_bins = 19;
static const float age_bin_values[] = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95};
static const std::vector<float> actual_age_bins ( age_bin_values, age_bin_values + sizeof(age_bin_values) / sizeof(age_bin_values[0]) );
static std::vector<string> age_bins_names;
//channels by age, note they have the same names but MUST HAVE A SPACE AT THE END
static std::string _stat_pop_by_age_label                 = "Statistical Population ";
static std::string _latent_TB_by_age_label                = "Latent TB Population ";
static std::string _latent_TB_fast_by_age_label           = "Latent TB Fast Population ";
static std::string _latent_TB_slow_by_age_label           = "Latent TB Slow Population ";
static std::string _new_active_by_age_label               = "New Active TB Infections ";
static std::string _active_TB_by_age_label                = "Active TB Population "; //new
static std::string _no_active_TB_by_age_label             = "No Active TB Population "; //new
static std::string _disease_deaths_by_age_label           = "Disease Deaths "; //new
static std::string _stat_pop_deaths_by_age_label          = "Deaths ";
static std::string _active_smearpos_TB_by_age_label       = "Active Smearpos TB Population "; 

//initialize your demog bins (defined in the ctor) 
static const int _num_agegroups_bins                      = 2;
static std::vector<std::string> agegroups_bins;
//channels by adult/child (manually defined), note they have the same names but MUST HAVE A SPACE AT THE END
static std::string _stat_pop_by_agegroups_label           = "Statistical Population "; 
static std::string _disease_deaths_by_agegroups_label     = "Disease Deaths "; 
static std::string _active_TB_by_agegroups_label          = "Active TB Population "; 
static std::string _latent_TB_by_agegroups_label          = "Latent TB Population ";
static std::string _latent_TB_fast_by_agegroups_label     = "Latent TB Fast Population ";
static std::string _latent_TB_slow_by_agegroups_label     = "Latent TB Slow Population ";
static std::string _new_active_by_agegroups_label         = "New Active TB Infections ";
static std::string _active_smearpos_TB_by_agegroups_label = "Active Smearpos TB Population "; 
static std::string _immune_by_agegroups_label             = "Immune Population ";


//initialize your demog bins (defined in the ctor) 
static const int _num_demog_bins                          = 4;
static std::vector<string> demog_bins;
//channels by adult/child (manually defined), note they have the same names but MUST HAVE A SPACE AT THE END
static std::string _stat_pop_by_demog_label               = "Statistical Population "; 
static std::string _disease_deaths_by_demog_label         = "Disease Deaths "; 
static std::string _active_TB_by_demog_label              = "Active TB Population "; 
static std::string _latent_TB_by_demog_label              = "Latent TB Population ";
static std::string _latent_TB_fast_by_demog_label         = "Latent TB Fast Population ";
static std::string _latent_TB_slow_by_demog_label         = "Latent TB Slow Population ";
static std::string _new_active_by_demog_label             = "New Active TB Infections ";
static std::string _active_smearpos_TB_by_demog_label     = "Active Smearpos TB Population "; 

//initialize your property bins (manually defined in the ctor) 
static const int _num_prop_bins                           = 3;
static std::vector<string> prop_bins;
//channels by property, note you can aggregate some of the properties since it is MANUALLY DEFINED,
//titles MUST HAVE A SPACE AT THE END
static std::string _stat_pop_by_prop_label                = "Statistical Population "; 
static std::string _disease_deaths_by_prop_label          = "Disease Deaths "; 
static std::string _active_TB_by_prop_label               = "Active TB Population "; 
static std::string _latent_TB_by_prop_label               = "Latent TB Population ";
static std::string _latent_TB_fast_by_prop_label          = "Latent TB Fast Population ";
static std::string _latent_TB_slow_by_prop_label          = "Latent TB Slow Population ";
static std::string _latent_TB_pendingrelapse_by_prop_label= "Latent PendingRelapse TB Population ";

static std::string _new_active_by_prop_label              = "New Active TB Infections ";
static std::string _active_smearpos_TB_by_prop_label      = "Active Smearpos TB Population "; 
static std::string _active_TB_mdr_by_prop_label           = "Active TB Population MDR ";
static std::string _active_TB_naive_by_prop_label         = "Active TB Population Naive ";
static std::string _active_TB_retx_by_prop_label          = "Active TB Population Retx ";
static std::string _active_TB_naive_mdr_by_prop_label     = "Active TB Population Naive MDR ";
static std::string _active_TB_retx_mdr_by_prop_label      = "Active TB Population Retx MDR ";
static std::string _immune_by_prop_label                  = "Immune Population ";

static const std::string _adult_agegroup_label       = "Adult";
static const std::string _child_agegroup_label       = "Child";
static const std::string _early_adult_agegroup_label = "EarlyAdult";
static const std::string _early_child_agegroup_label = "EarlyChild";
static const std::string _late_adult_agegroup_label  = "LateAdult";
static const std::string _late_child_agegroup_label  = "LateChild";

static const std::string _public_prop_label =  "Public";
static const std::string _private_prop_label = "Private";
static const std::string _none_prop_label =    "None";

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
// --- Report_Scenarios Methods
// ----------------------------------------

#define TB_REPORTING_AGE_LIMIT (15.0f)
#define INIT_CHAN_WITH_ZERO(x) Accumulate( x, 0 )
#define REPORTING_PERIOD_IN_TSTEPS (5)
#define REPORTING_START_TIME (1)

Report_Scenarios::Report_Scenarios()
: BaseChannelReport()
, last_years_births(0)
, this_years_births(0)
, _countupToNextPeriodicTarget( REPORTING_PERIOD_IN_TSTEPS ) //it accumulates data every timestep, if you want a yearly report use countupToNextPeriodicReport = DAYSPERYEAR
, countupToNextPeriodicReport( REPORTING_START_TIME ) // start counting time from 1 (the increment is below)
, ntic_list()
{
    LOG_DEBUG( "Report_Scenarios ctor\n" );
    report_name = _report_name;
    countupToNextPeriodicReport = (*EnvPtr->Config)["Simulation_Timestep"].As<Number>();
    
    //automatically build the age bin names from the age_bin_values entered above
    float lower_bound = 0;
    for( auto age : age_bin_values )
    {
        std::ostringstream label;
        LOG_INFO_F("age is %f \n", age);

        float upper_bound = age-1;
        label << lower_bound << "-" << upper_bound;

        //put the label into the age_bins_names list and get ready for the next round
        age_bins_names.push_back( label.str() );
        LOG_INFO_F( "added age label %s.\n", label.str().c_str() );
        lower_bound = age;
        LOG_INFO_F("first age is %f.\n", age_bin_values[0]);
        LOG_INFO_F("first age int is %d.\n", (int) age_bin_values[0]);
        LOG_INFO_F("last age is %f.\n", age_bin_values[_num_age_bins - 1]);
        //special case for oldest age
        if (lower_bound == age_bin_values[_num_age_bins-1])
        {
            std::ostringstream label;
            label << lower_bound << "+";
            age_bins_names.push_back( label.str() );
            LOG_INFO_F("added age label %s.\n", label.str().c_str());
        }
    }
    agegroups_bins.push_back( _adult_agegroup_label );
    agegroups_bins.push_back( _child_agegroup_label );

    //here is the list of demog_bins names
    demog_bins.push_back(_early_adult_agegroup_label);
    demog_bins.push_back(_late_adult_agegroup_label);
    demog_bins.push_back(_early_child_agegroup_label);
    demog_bins.push_back(_late_child_agegroup_label);

    //here is the list of prop_bins names
    prop_bins.push_back(_public_prop_label);
    prop_bins.push_back(_private_prop_label);
    prop_bins.push_back(_none_prop_label);

    last_years_births = 0;
    this_years_births = 0;
    //Initialize all channels with zero so that they will certainly all show up in graphs later
    INIT_CHAN_WITH_ZERO ( _stat_pop_label );
    INIT_CHAN_WITH_ZERO ( _stat_pop_births_label );
    INIT_CHAN_WITH_ZERO ( _stat_pop_deaths_label );
    INIT_CHAN_WITH_ZERO ( _prob_new_inf_label );
    INIT_CHAN_WITH_ZERO ( _new_active_label );
    INIT_CHAN_WITH_ZERO ( _disease_deaths_label );
    INIT_CHAN_WITH_ZERO ( _new_active_mdr_label );
    INIT_CHAN_WITH_ZERO ( _new_active_acquired_mdr_label );
    INIT_CHAN_WITH_ZERO ( _new_active_fast_label );
    INIT_CHAN_WITH_ZERO ( _new_active_twoyrs_label );
    INIT_CHAN_WITH_ZERO ( _latent_TB_label );
    INIT_CHAN_WITH_ZERO ( _latent_TB_fast_label );
    INIT_CHAN_WITH_ZERO ( _latent_TB_slow_label );
    INIT_CHAN_WITH_ZERO ( _latent_ontx_label );
    INIT_CHAN_WITH_ZERO ( _active_TB_label );   
    INIT_CHAN_WITH_ZERO ( _active_TB_mdr_label );
    INIT_CHAN_WITH_ZERO ( _active_TB_acquired_mdr_label );
    INIT_CHAN_WITH_ZERO ( _active_TB_naive_label );
    INIT_CHAN_WITH_ZERO ( _active_TB_retx_label );
    INIT_CHAN_WITH_ZERO ( _active_TB_naive_mdr_label );
    INIT_CHAN_WITH_ZERO ( _active_TB_retx_mdr_label );
    INIT_CHAN_WITH_ZERO ( _active_TB_empirictx_label );
    INIT_CHAN_WITH_ZERO ( _active_TB_secondlinecombo_label );
    INIT_CHAN_WITH_ZERO ( _active_TB_empirictx_mdr_label );
    INIT_CHAN_WITH_ZERO ( _active_TB_secondlinecombo_mdr_label );
    INIT_CHAN_WITH_ZERO ( _secondlinecombo_label );
    INIT_CHAN_WITH_ZERO ( _no_active_TB_label );
    INIT_CHAN_WITH_ZERO ( _active_smearpos_TB_label );
    INIT_CHAN_WITH_ZERO ( _num_tbactivation_label );
    INIT_CHAN_WITH_ZERO ( _num_providerorderstbtest_label );
    INIT_CHAN_WITH_ZERO ( _num_tbtestdefault_label );
    INIT_CHAN_WITH_ZERO ( _num_tbtestnegative_label );
    INIT_CHAN_WITH_ZERO ( _num_tbtestpositive_label );
    INIT_CHAN_WITH_ZERO ( _num_tbmdrtestdefault_label );
    INIT_CHAN_WITH_ZERO ( _num_tbmdrtestnegative_label );
    INIT_CHAN_WITH_ZERO ( _num_tbmdrtestpositive_label );
    INIT_CHAN_WITH_ZERO ( _num_tbstartdrugregimen_label );
    INIT_CHAN_WITH_ZERO ( _num_naive_tbstartdrugregimen_label );
    INIT_CHAN_WITH_ZERO ( _num_retx_tbstartdrugregimen_label );
    INIT_CHAN_WITH_ZERO ( _num_tbfaileddrugregimen_label );

    for( auto age_bin : age_bins_names )
    {
        LOG_INFO_F("age bin initializer name %s \n", age_bin.c_str());
        INIT_CHAN_WITH_ZERO( _stat_pop_by_age_label           + age_bin );
        INIT_CHAN_WITH_ZERO( _stat_pop_deaths_by_age_label    + age_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_by_age_label          + age_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_fast_by_age_label     + age_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_slow_by_age_label     + age_bin );
        INIT_CHAN_WITH_ZERO( _new_active_by_age_label         + age_bin );
        INIT_CHAN_WITH_ZERO( _active_TB_by_age_label          + age_bin );
        INIT_CHAN_WITH_ZERO( _no_active_TB_by_age_label       + age_bin );
        INIT_CHAN_WITH_ZERO( _disease_deaths_by_age_label     + age_bin );    
        INIT_CHAN_WITH_ZERO( _active_smearpos_TB_by_age_label + age_bin );
    }
    
    for( auto agegroups_bin : agegroups_bins)
    {
        LOG_INFO_F("agegroups_bin initializer name %s \n", agegroups_bin.c_str());
        INIT_CHAN_WITH_ZERO( _stat_pop_by_agegroups_label           + agegroups_bin );
        INIT_CHAN_WITH_ZERO( _disease_deaths_by_agegroups_label     + agegroups_bin );
        INIT_CHAN_WITH_ZERO( _active_TB_by_agegroups_label          + agegroups_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_by_agegroups_label          + agegroups_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_fast_by_agegroups_label     + agegroups_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_slow_by_agegroups_label     + agegroups_bin );
        INIT_CHAN_WITH_ZERO( _new_active_by_agegroups_label         + agegroups_bin );
        INIT_CHAN_WITH_ZERO( _active_smearpos_TB_by_agegroups_label + agegroups_bin );
        INIT_CHAN_WITH_ZERO( _immune_by_agegroups_label             + agegroups_bin );
    }

    for( auto demog_bin : demog_bins)
    {
        LOG_INFO_F("demog bin initializer name %s \n", demog_bin.c_str());
        INIT_CHAN_WITH_ZERO( _stat_pop_by_demog_label           + demog_bin );
        INIT_CHAN_WITH_ZERO( _disease_deaths_by_demog_label     + demog_bin );
        INIT_CHAN_WITH_ZERO( _active_TB_by_demog_label          + demog_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_by_demog_label          + demog_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_fast_by_demog_label     + demog_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_slow_by_demog_label     + demog_bin );
        INIT_CHAN_WITH_ZERO( _new_active_by_demog_label         + demog_bin );
        INIT_CHAN_WITH_ZERO( _active_smearpos_TB_by_demog_label + demog_bin );
    }

    for( auto prop_bin : prop_bins)
    {
        LOG_INFO_F("prop bin initializer name %s \n", prop_bin.c_str());
        INIT_CHAN_WITH_ZERO( _stat_pop_by_prop_label                 + prop_bin );
        INIT_CHAN_WITH_ZERO( _disease_deaths_by_prop_label           + prop_bin );
        INIT_CHAN_WITH_ZERO( _active_TB_by_prop_label                + prop_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_by_prop_label                + prop_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_fast_by_prop_label           + prop_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_slow_by_prop_label           + prop_bin );
        INIT_CHAN_WITH_ZERO( _new_active_by_prop_label               + prop_bin );
        INIT_CHAN_WITH_ZERO( _active_smearpos_TB_by_prop_label       + prop_bin );
        INIT_CHAN_WITH_ZERO( _latent_TB_pendingrelapse_by_prop_label + prop_bin );
        INIT_CHAN_WITH_ZERO( _active_TB_mdr_by_prop_label            + prop_bin );
        INIT_CHAN_WITH_ZERO( _active_TB_naive_by_prop_label          + prop_bin );
        INIT_CHAN_WITH_ZERO( _active_TB_retx_by_prop_label           + prop_bin );
        INIT_CHAN_WITH_ZERO( _active_TB_naive_mdr_by_prop_label      + prop_bin );
        INIT_CHAN_WITH_ZERO( _active_TB_retx_mdr_by_prop_label       + prop_bin );
        INIT_CHAN_WITH_ZERO( _immune_by_prop_label                   + prop_bin );
    }

    // ------------------------------------------------------------------------------------------------
    // --- Since this report will be listening for events, it needs to increment its reference count
    // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
    // --- so it needs to start with a refcount of 1.
    // ------------------------------------------------------------------------------------------------
    AddRef();
}

Report_Scenarios::~Report_Scenarios()
{ 
}


/////////////////////////
// steady-state methods
/////////////////////////
void
Report_Scenarios::LogNodeData(
    INodeContext * pNC
)
{
    LOG_DEBUG( "LogNodeData.\n" );
    BaseChannelReport::LogNodeData( pNC );

    Accumulate(_stat_pop_label, pNC->GetStatPop());
    this_years_births = pNC->GetBirths();
    Accumulate(_prob_new_inf_label,   pNC->GetInfectionRate());

    // get pointer to INodeTB
    INodeTB* pTBNode = nullptr;
    if( pNC->QueryInterface( GET_IID(INodeTB), (void**)&pTBNode ) != s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNode", "INodeTB", "INodeContext" );
    }
    release_assert(pTBNode);

    Accumulate(_new_active_label, pTBNode->GetIncidentCounter() );

    //need to include all the individualeventtriggertypes that will be used in logindividualdata. 
    //This happens once at the beginning, but is here (not in the ctor because you need the Node pointer)
    if( pNC->GetTime() < (*EnvPtr->Config)["Simulation_Timestep"].As<Number>() )
    {
        LOG_INFO_F( "Instantiating observers at first timestep \n" );
        // Normally our interventions are created from json. Might actually be easier
        INodeEventContext * pNEC = pNC->GetEventContext();
        release_assert( pNEC );

        INodeTriggeredInterventionConsumer* pNTIC = nullptr;
        if( pNEC->QueryInterface( GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNEC", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
        }
        release_assert( pNTIC );

        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::TBActivation         );
        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::ProviderOrdersTBTest );
        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::TBTestDefault        );
        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::TBTestNegative       );
        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::TBTestPositive       );
        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::TBMDRTestDefault     );
        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::TBMDRTestNegative    );
        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::TBMDRTestPositive    );
        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::TBStartDrugRegimen   );
        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::TBFailedDrugRegimen  );

        ntic_list.push_back( pNTIC );
     }
}

int Report_Scenarios::calcBinIndex(const IndividualHuman * individual)
{
    float age = (float)individual->GetAge();

    // Calculate age bin
    int agebin = lower_bound( actual_age_bins.begin(), actual_age_bins.end(), age/DAYSPERYEAR ) - actual_age_bins.begin();
    int bin_index = agebin;
    release_assert(agebin<=_num_age_bins);
    LOG_VALID_F("Bin index is %d \n", bin_index);
    return bin_index;
}

void 
Report_Scenarios::LogIndividualData(
    IndividualHuman * individual
)
{
    // get monte carlo weight
    float mc_weight = (float)individual->GetMonteCarloWeight();

    // get pointer to IIndividualHumanTB2 (full TB interface) and the person's SusceptibilityTB
    IIndividualHumanTB2* individual_tb = nullptr;
    if( individual->QueryInterface( GET_IID(IIndividualHumanTB2), (void**)&individual_tb ) != s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHumanTB2", "IndividualHuman" );
    }
    release_assert( individual_tb );

    //get some info about this person's age and their age bin
    int age_bin_index = calcBinIndex(individual);

    // figure out if individual has drugtype LatentTreatment
    bool onLatentTreatment  = false;
    bool onDOTS             = false;
    bool onMDRTx            = false;
    bool onEmpiricTreatment = false;
    IIndividualHumanInterventionsContext * intervs = individual->GetInterventionsContext();
    ITBDrugEffects* pTBDrugEffects = nullptr;
    if( intervs->QueryInterface( GET_IID(ITBDrugEffects), (void**)&pTBDrugEffects ) != s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "intervs", "ITBDrugEffects", "IIndividualHumanInterventionsContext" );
    }
    TBDrugEffectsMap_t TB_drug_effects = pTBDrugEffects->GetDrugEffectsMap();
    //add up all drug effects
    for (auto& drug_effect : TB_drug_effects)
    {
        onLatentTreatment |= (drug_effect.first == TBDrugType::LatentTreatment );
        onDOTS |= (drug_effect.first == TBDrugType::DOTS);
        onMDRTx |= (drug_effect.first == TBDrugType::SecondLineCombo);
        onEmpiricTreatment |= (drug_effect.first == TBDrugType::EmpiricTreatment);
    }

    //get some info about this person's demog bin
    IIndividualHumanTB* individual_tb_direct = nullptr;
    if( individual->QueryInterface( GET_IID(IIndividualHumanTB), (void**)&individual_tb_direct ) != s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHumanTB", "IndividualHuman" );
    }
    release_assert( individual_tb_direct );

    // ------------------------------------------------------------------------------
    // --- We are making pProp const because we do not want to change its value.
    // --- This will keep us from using the "[]" operator which has the side affect
    // --- of modifying the propeties.  We need the "count" check before access since
    // --- ".at" will throw an exception if the value is not found in the map.
    // ------------------------------------------------------------------------------
    const tProperties* pProp = individual->GetEventContext()->GetProperties();
    float age = (float)individual->GetAge();
    float age_limit = TB_REPORTING_AGE_LIMIT;

    // string literals used more than once get a variable
    const std::string qoc_prop_key = "QualityOfCare";
    std::string qoc_prop_str = ((*pProp).count( qoc_prop_key ) > 0) ? (*pProp).at( qoc_prop_key ) : "-none-" ; 
    LOG_DEBUG_F( "Person is %f, QualityOfCare %s\n", age, qoc_prop_str.c_str() );

    std::string agegroups_bin_name = "";
    if ( age > age_limit * DAYSPERYEAR ) 
    {
        agegroups_bin_name = _adult_agegroup_label;
    }
    else 
    {
        agegroups_bin_name = _child_agegroup_label;
    }

    std::string demog_bin_name = "";
    if( ((*pProp).count("Access") > 0) && ((*pProp).at("Access") == std::string("Early")) )
    {
        if ( age > age_limit * DAYSPERYEAR ) 
        {
            demog_bin_name = _early_adult_agegroup_label;
        }
        else 
        {
            demog_bin_name = _early_child_agegroup_label;
        }
    }
    else
    {
        if ( age > age_limit * DAYSPERYEAR ) 
        {
            demog_bin_name = _late_adult_agegroup_label;
        }
        else 
        {
            demog_bin_name = _late_child_agegroup_label;
        }
    }

    //get some info about this person's prop bin
    std::string prop_bin_name = "";
    if( qoc_prop_str == std::string("CDC") ||
        qoc_prop_str == _public_prop_label ) 
    {
        prop_bin_name = _public_prop_label;
    }
    else if( qoc_prop_str == std::string("Hospital") ||
             qoc_prop_str == std::string(_private_prop_label) ) 
    {
        prop_bin_name = _private_prop_label;
    }
    else
    {
        prop_bin_name = _none_prop_label;
    }

    //now time to accumulate the data 
    auto age_label   = _stat_pop_by_age_label   + age_bins_names[age_bin_index];
    auto demog_label = _stat_pop_by_demog_label + demog_bin_name;
    auto prop_label  = _stat_pop_by_prop_label  + prop_bin_name;
    
    Accumulate( _stat_pop_by_age_label       + age_bins_names[age_bin_index], mc_weight );
    Accumulate( _stat_pop_by_demog_label     + demog_bin_name,                mc_weight );
    Accumulate( _stat_pop_by_prop_label      + prop_bin_name,                 mc_weight );
    Accumulate( _stat_pop_by_agegroups_label + agegroups_bin_name,            mc_weight );


    if (individual_tb->IsImmune() ) 
    {
        Accumulate( _immune_by_agegroups_label + agegroups_bin_name, mc_weight );
        Accumulate( _immune_by_prop_label      + prop_bin_name,      mc_weight );
    }

    if(individual->GetStateChange() == HumanStateChange::DiedFromNaturalCauses)
    {
        Accumulate( _stat_pop_deaths_label,                                        mc_weight );    
        Accumulate( _stat_pop_deaths_by_age_label + age_bins_names[age_bin_index], mc_weight );
    }

    if ( individual->GetStateChange() == HumanStateChange::KilledByInfection )
    {
        Accumulate( _disease_deaths_label,                                              mc_weight );
        Accumulate( _disease_deaths_by_age_label       + age_bins_names[age_bin_index], mc_weight );    
        Accumulate( _disease_deaths_by_demog_label     + demog_bin_name,                mc_weight );
        Accumulate( _disease_deaths_by_prop_label      + prop_bin_name,                 mc_weight );
        Accumulate( _disease_deaths_by_agegroups_label + agegroups_bin_name,            mc_weight );
    }

    if ( individual->GetNewInfectionState() == NewInfectionState::NewlyActive )
    {
        Accumulate( _new_active_by_age_label       + age_bins_names[age_bin_index], mc_weight );    
        Accumulate( _new_active_by_demog_label     + demog_bin_name,                mc_weight );
        Accumulate( _new_active_by_prop_label      + prop_bin_name,                 mc_weight );
        Accumulate( _new_active_by_agegroups_label + agegroups_bin_name,            mc_weight );

        if ( individual_tb->IsFastProgressor() )
        {
            Accumulate(_new_active_fast_label, mc_weight );            
        }

        if ( individual_tb->IsMDR() ) 
        {
            Accumulate(_new_active_mdr_label, mc_weight );
            if ( individual_tb->IsEvolvedMDR() )
            {
                Accumulate(_new_active_acquired_mdr_label, mc_weight );
            }
        }

        if ( individual_tb->GetDurationSinceInitInfection() < (DAYSPERYEAR*2) ) 
        {
            Accumulate(_new_active_twoyrs_label, mc_weight );                    
        }
    }    

    if ( individual_tb->HasLatentInfection() )
    {
        Accumulate( _latent_TB_label,                                              mc_weight );    
        Accumulate( _latent_TB_by_age_label       + age_bins_names[age_bin_index], mc_weight );    
        Accumulate( _latent_TB_by_demog_label     + demog_bin_name,                mc_weight );
        Accumulate( _latent_TB_by_prop_label      + prop_bin_name,                 mc_weight );
        Accumulate( _latent_TB_by_agegroups_label + agegroups_bin_name,            mc_weight );

        if ( individual_tb->IsFastProgressor() )
        {
            Accumulate(_latent_TB_fast_label, mc_weight );    
            Accumulate( _latent_TB_fast_by_age_label       + age_bins_names[age_bin_index], mc_weight );    
            Accumulate( _latent_TB_fast_by_demog_label     + demog_bin_name,                mc_weight );
            Accumulate( _latent_TB_fast_by_prop_label      + prop_bin_name,                 mc_weight );
            Accumulate( _latent_TB_fast_by_agegroups_label + agegroups_bin_name,            mc_weight );
        }
        else
        {
            Accumulate(_latent_TB_slow_label, mc_weight );    
            Accumulate( _latent_TB_slow_by_age_label       + age_bins_names[age_bin_index], mc_weight );    
            Accumulate( _latent_TB_slow_by_demog_label     + demog_bin_name,                mc_weight );
            Accumulate( _latent_TB_slow_by_prop_label      + prop_bin_name,                 mc_weight );
            Accumulate( _latent_TB_slow_by_agegroups_label + agegroups_bin_name,            mc_weight );        
        }

        if ( onLatentTreatment && !individual_tb->HasPendingRelapseInfection() )
        {
            Accumulate(_latent_ontx_label, mc_weight );
        }

        if (individual_tb->HasPendingRelapseInfection() ) 
        {
            Accumulate( _latent_TB_pendingrelapse_by_prop_label + prop_bin_name, mc_weight );
        }
    }

    if ( individual_tb->HasActiveInfection() )
    {
        Accumulate( _active_TB_label, mc_weight);
        Accumulate( _active_TB_by_age_label       + age_bins_names[age_bin_index], mc_weight );    
        Accumulate( _active_TB_by_demog_label     + demog_bin_name,                mc_weight );
        Accumulate( _active_TB_by_prop_label      + prop_bin_name,                 mc_weight );
        Accumulate( _active_TB_by_agegroups_label + agegroups_bin_name,            mc_weight );

        if ( individual_tb->IsSmearPositive() ) 
        {
            Accumulate( _active_smearpos_TB_label, mc_weight );
            Accumulate( _active_smearpos_TB_by_age_label       + age_bins_names[age_bin_index], mc_weight );    
            Accumulate( _active_smearpos_TB_by_demog_label     + demog_bin_name,                mc_weight );
            Accumulate( _active_smearpos_TB_by_prop_label      + prop_bin_name,                 mc_weight );
            Accumulate( _active_smearpos_TB_by_agegroups_label + agegroups_bin_name,            mc_weight );
        }

        if ( individual_tb->IsMDR() ) 
        {
            Accumulate( _active_TB_mdr_label,                         mc_weight ); 
            Accumulate( _active_TB_mdr_by_prop_label + prop_bin_name, mc_weight );
            
            if ( individual_tb->IsEvolvedMDR() ) 
            {
                Accumulate( _active_TB_acquired_mdr_label, mc_weight );          
            }

            if ( individual_tb->IsTreatmentNaive() && !individual_tb->IsOnTreatment()) 
            {
                Accumulate( _active_TB_naive_mdr_label,                         mc_weight ); 
                Accumulate( _active_TB_naive_mdr_by_prop_label + prop_bin_name, mc_weight );
            }        
            else if ( (individual_tb->HasFailedTreatment() || individual_tb->HasEverRelapsedAfterTreatment() )  && !individual_tb->IsOnTreatment())
            {
                Accumulate( _active_TB_retx_mdr_label,                         mc_weight ); 
                Accumulate( _active_TB_retx_mdr_by_prop_label + prop_bin_name, mc_weight );
            }
        }

        if ( onEmpiricTreatment ) 
        {
            Accumulate( _active_TB_empirictx_label, mc_weight ); 
            if ( individual_tb->IsMDR() )
            {
                Accumulate( _active_TB_empirictx_mdr_label, mc_weight );               
            }
        }

        if ( onMDRTx )
        {
            Accumulate( _active_TB_secondlinecombo_label, mc_weight ); 
            if ( individual_tb->IsMDR() )
            {
                Accumulate( _active_TB_secondlinecombo_mdr_label, mc_weight );               
            }        
        }

        if ( individual_tb->IsTreatmentNaive() && !individual_tb->IsOnTreatment()) 
        {
            Accumulate( _active_TB_naive_label,                         mc_weight ); 
            Accumulate( _active_TB_naive_by_prop_label + prop_bin_name, mc_weight );

        }        
        else if ( (individual_tb->HasFailedTreatment() || individual_tb->HasEverRelapsedAfterTreatment() )  && !individual_tb->IsOnTreatment())
        {
            Accumulate (_active_TB_retx_label,                         mc_weight ); 
            Accumulate( _active_TB_retx_by_prop_label + prop_bin_name, mc_weight );
        }

    }
    else
    {
        Accumulate( _no_active_TB_label,                                        mc_weight);  
        Accumulate( _no_active_TB_by_age_label + age_bins_names[age_bin_index], mc_weight );    
    }

    if ( onMDRTx ) 
    {
        Accumulate (_secondlinecombo_label,                         mc_weight ); 
    }
}

// Just calling the base class but for demo purposes leaving in because I can imagine wanting to do this custom.
void
Report_Scenarios::Finalize()
{
    for( auto p_ntic : ntic_list )
    {
        p_ntic->UnregisterNodeEventObserver( this, IndividualEventTriggerType::TBActivation         );
        p_ntic->UnregisterNodeEventObserver( this, IndividualEventTriggerType::ProviderOrdersTBTest );
        p_ntic->UnregisterNodeEventObserver( this, IndividualEventTriggerType::TBTestDefault        );
        p_ntic->UnregisterNodeEventObserver( this, IndividualEventTriggerType::TBTestNegative       );
        p_ntic->UnregisterNodeEventObserver( this, IndividualEventTriggerType::TBTestPositive       );
        p_ntic->UnregisterNodeEventObserver( this, IndividualEventTriggerType::TBMDRTestDefault     );
        p_ntic->UnregisterNodeEventObserver( this, IndividualEventTriggerType::TBMDRTestNegative    );
        p_ntic->UnregisterNodeEventObserver( this, IndividualEventTriggerType::TBMDRTestPositive    );
        p_ntic->UnregisterNodeEventObserver( this, IndividualEventTriggerType::TBStartDrugRegimen   );
        p_ntic->UnregisterNodeEventObserver( this, IndividualEventTriggerType::TBFailedDrugRegimen  );
    }

    LOG_INFO( "WriteData\n" );
    return BaseChannelReport::Finalize();
}

void 
Report_Scenarios::populateSummaryDataUnitsMap(
    std::map<std::string, std::string> &units_map
)
{
    LOG_INFO( "populateSummaryDataUnitsMap\n" );
    units_map[_stat_pop_label]                      = "Population";
    units_map[_stat_pop_births_label]               = 
    units_map[_stat_pop_deaths_label]               = "Population";
    units_map[_prob_new_inf_label]                  = "Infection Rate";
    units_map[_new_active_label]                    =
    units_map[_disease_deaths_label]                =
    units_map[_new_active_mdr_label]                =
    units_map[_new_active_acquired_mdr_label]       =
    units_map[_new_active_fast_label]               =
    units_map[_new_active_twoyrs_label]             = "Individuals per year";
    units_map[_latent_TB_label]                     =
    units_map[_latent_TB_fast_label]                =
    units_map[_latent_TB_slow_label]                = "Population latently infected";
    units_map[_latent_ontx_label]                   = "Fraction latent on treatment";
    units_map[_active_TB_label]                     = "Population actively infected";
    units_map[_active_TB_mdr_label]                 = "Population actively infected who have MDR";
    units_map[_active_TB_acquired_mdr_label]        = "Population actively infected who have acquired MDR";
    units_map[_active_TB_naive_label]               = "Population actively infected Tx Naive";
    units_map[_active_TB_retx_label]                = "Population actively infected Retx";
    units_map[_active_TB_naive_mdr_label]           = "Population actively infected Tx Naive MDR";
    units_map[_active_TB_retx_mdr_label]            = "Population actively infected Retx MDR";
    units_map[_active_TB_empirictx_label]           = "Population actively infected on EmpiricTreatment";
    units_map[_active_TB_secondlinecombo_label]     = "Population actively infected on SecondLineCombo";
    units_map[_active_TB_empirictx_mdr_label]       = "Population actively infected MDR on EmpiricTreatment";
    units_map[_active_TB_secondlinecombo_mdr_label] = "Population actively infected MDR on SecondLineCombo";
    units_map[_active_TB_secondlinecombo_mdr_label] = "Population on SecondLineCombo";
    units_map[_no_active_TB_label]                  = "Population not actively infected";
    units_map[_active_smearpos_TB_label]            = "Population actively infected";

    for( auto age_bin : age_bins_names )
    {
        units_map[_stat_pop_by_age_label           + age_bin] =
        units_map[_stat_pop_deaths_by_age_label    + age_bin] = "Population in age bin";
        units_map[_disease_deaths_by_age_label     + age_bin] = "Disease Deaths in Age Bin";
        units_map[_latent_TB_by_age_label          + age_bin] =
        units_map[_latent_TB_fast_by_age_label     + age_bin] =
        units_map[_latent_TB_slow_by_age_label     + age_bin] = "Latent Population in Age Bin";
        units_map[_new_active_by_age_label         + age_bin] = "Individuals per year";
        units_map[_no_active_TB_by_age_label       + age_bin] = "No Active Population in Age Bin";
        units_map[_active_TB_by_age_label          + age_bin] =
        units_map[_active_smearpos_TB_by_age_label + age_bin] = "Active Population in Age Bin";
    }

    for( auto agegroups_bin : agegroups_bins)
    {
        units_map[_stat_pop_by_agegroups_label           + agegroups_bin] = "Population in Demog Bin";
        units_map[_disease_deaths_by_agegroups_label     + agegroups_bin] = "Disease Deaths in Age Bin";
        units_map[_latent_TB_by_agegroups_label          + agegroups_bin] =
        units_map[_latent_TB_fast_by_agegroups_label     + agegroups_bin] =
        units_map[_latent_TB_slow_by_agegroups_label     + agegroups_bin] = "Latent Population in Demog Bin";
        units_map[_new_active_by_agegroups_label         + agegroups_bin] = "Individuals per year";
        units_map[_active_TB_by_agegroups_label          + agegroups_bin] =
        units_map[_active_smearpos_TB_by_agegroups_label + agegroups_bin] = "Active Population in Demog Bin";
        units_map[_immune_by_agegroups_label             + agegroups_bin] = "Immune Population in Demog Bin";
    }

    for( auto demog_bin : demog_bins)
    {
        units_map[_stat_pop_by_demog_label           + demog_bin] = "Population in Demog Bin";
        units_map[_disease_deaths_by_demog_label     + demog_bin] = "Disease Deaths in Age Bin";
        units_map[_latent_TB_by_demog_label          + demog_bin] = 
        units_map[_latent_TB_fast_by_demog_label     + demog_bin] = 
        units_map[_latent_TB_slow_by_demog_label     + demog_bin] = "Latent Population in Demog Bin";
        units_map[_new_active_by_demog_label         + demog_bin] = "Individuals per year";
        units_map[_active_TB_by_demog_label          + demog_bin] = 
        units_map[_active_smearpos_TB_by_demog_label + demog_bin] = "Active Population in Demog Bin";
    }
       
    for( auto prop_bin : prop_bins)
    {
        units_map[_stat_pop_by_prop_label                 + prop_bin] = "Population in Prop Bin";
        units_map[_new_active_by_prop_label               + prop_bin] = "Individuals per year";
        units_map[_disease_deaths_by_prop_label           + prop_bin] = "Disease Deaths in Age Bin";
        units_map[_latent_TB_by_prop_label                + prop_bin] =
        units_map[_latent_TB_fast_by_prop_label           + prop_bin] =
        units_map[_latent_TB_slow_by_prop_label           + prop_bin] =
        units_map[_latent_TB_pendingrelapse_by_prop_label + prop_bin] = "Latent Population in Prop Bin";
        units_map[_active_TB_by_prop_label                + prop_bin] =
        units_map[_active_smearpos_TB_by_prop_label       + prop_bin] =
        units_map[_active_TB_mdr_by_prop_label            + prop_bin] =
        units_map[_active_TB_naive_by_prop_label          + prop_bin] =
        units_map[_active_TB_retx_by_prop_label           + prop_bin] =
        units_map[_active_TB_naive_mdr_by_prop_label      + prop_bin] =
        units_map[_active_TB_retx_mdr_by_prop_label       + prop_bin] = "Active Population in Prop Bin";
        units_map[_immune_by_prop_label                   + prop_bin] = "Immune Population in Prop Bin";

    }
}

// not sure whether to leave this in custom demo subclass
void
Report_Scenarios::postProcessAccumulatedData()
{
    LOG_DEBUG( "getSummaryDataCustomProcessed\n" );
    normalizeChannel(_prob_new_inf_label, (float)_nrmSize);
}

void
Report_Scenarios::BeginTimestep()
{
}

void
Report_Scenarios::EndTimestep( float currentTime, float dt )
{
    LOG_DEBUG("start end timestep \n");
    LOG_DEBUG_F( "countupToNextPeriodicReport = %d _countupToNextPeriodicTarget = %d.\n", countupToNextPeriodicReport, _countupToNextPeriodicTarget );

    //increment your countup counter by the simulation timestep
    if( countupToNextPeriodicReport < _countupToNextPeriodicTarget )
    {
        //get the simulation timestep and add to the countupToNextPeriodicReportnt
        int sim_tstep = (*EnvPtr->Config)["Simulation_Timestep"].As<Number>();
        countupToNextPeriodicReport += sim_tstep;  
        LOG_DEBUG_F( "timestep is %d, new countupToNextPeriodicReport = %d \n", sim_tstep, countupToNextPeriodicReport);

        return;
    }
    //otherwise start your counter over again with timestep 
    countupToNextPeriodicReport = (*EnvPtr->Config)["Simulation_Timestep"].As<Number>();

    LOG_DEBUG_F( "Reached end of reporting period.\n" );

    std::vector<std::string> channel_names = channelDataMap.GetChannelNames();
    for( auto name : channel_names )
    {
        //add zero to all channels
        Accumulate(name, 0);
        if( name != "Timestep" )
        {
            if (name == "Births" ) 
            {
                Accumulate( _stat_pop_births_label, this_years_births - last_years_births);
                LOG_DEBUG_F("last_years_births %d, this_years_births %d \n", last_years_births, this_years_births);
                last_years_births = this_years_births;    
            }

            bool ChannelWillBeNormalized = true;

            //don't normalize these channels where you want the cumulative over the year
            if( name == _new_active_label 
                || name == _disease_deaths_label
                || name == _new_active_mdr_label
                || name == _new_active_acquired_mdr_label
                || name == _new_active_fast_label
                || name == _new_active_twoyrs_label
                || name == _prob_new_inf_label 
                || name == _num_tbactivation_label
                || name == _num_providerorderstbtest_label
                || name == _num_tbtestdefault_label
                || name == _num_tbtestnegative_label
                || name == _num_tbtestpositive_label
                || name == _num_tbstartdrugregimen_label
                || name == _num_naive_tbstartdrugregimen_label
                || name == _num_retx_tbstartdrugregimen_label
                || name == _num_tbfaileddrugregimen_label
                || name == _stat_pop_births_label
                || name == _stat_pop_deaths_label)
            {
                ChannelWillBeNormalized = false;
                LOG_DEBUG_F( "%s not normalized because this channel is the yearly sum \n", name.c_str() );
            }

            //these channels disaggregated by demog also should not be normalized, you want the cumulative over the year
            for( auto agegroups : agegroups_bins)
            {
                if( (name == std::string(_new_active_by_agegroups_label    ) + agegroups) ||
                    (name == std::string(_disease_deaths_by_agegroups_label) + agegroups) )
                {
                    ChannelWillBeNormalized = false;
                    LOG_DEBUG_F( "%s not normalized because this channel is the yearly sum \n", name.c_str() );
                }
            }

            for( auto demog : demog_bins)
            {
                if( (name == std::string(_new_active_by_demog_label    ) + demog) ||
                    (name == std::string(_disease_deaths_by_demog_label) + demog) )
                {
                    ChannelWillBeNormalized = false;
                    LOG_DEBUG_F( "%s not normalized because this channel is the yearly sum \n", name.c_str() );
                }
            }

            //these channels disaggregated by prop also should not be normalized, you want the cumulative over the year
            for( auto prop : prop_bins)
            {
                if( (name == std::string(_new_active_by_prop_label    ) + prop) ||
                    (name == std::string(_disease_deaths_by_prop_label) + prop) )
                {
                    ChannelWillBeNormalized = false;
                    LOG_DEBUG_F( "%s not normalized because this channel is the yearly sum \n", name.c_str() );
                }
            }

            //these channels disaggregated by age also should not be normalized, you want the cumulative over the year
            for( auto age : age_bins_names)
            {
                if( (name == std::string(_new_active_by_age_label     ) + age) ||
                    (name == std::string(_disease_deaths_by_age_label ) + age) || 
                    (name == std::string(_stat_pop_deaths_by_age_label) + age) )
                {
                    ChannelWillBeNormalized = false;
                    LOG_DEBUG_F( "%s not normalized because this channel is the yearly sum \n", name.c_str() );
                }
            }

            //all other channels get normalized, which means averaged across the year
            if ( ChannelWillBeNormalized )
            {
                const ChannelDataMap::channel_data_t& r_channel_data = channelDataMap.GetChannel( name );
                auto raw = r_channel_data.back();
                
                int sim_tstep = (*EnvPtr->Config)["Simulation_Timestep"].As<Number>();
               // this is if you want the year average: entry.second.back() /= (DAYSPERYEAR/sim_tstep) ;
                ChannelDataMap::channel_data_element_t normalized = raw / (_countupToNextPeriodicTarget/sim_tstep) ;
                channelDataMap.SetLastValue( name, normalized );
                LOG_DEBUG_F( "Dividing value %f in channel %s by %d to get %f\n", raw, name.c_str(), (_countupToNextPeriodicTarget/sim_tstep), normalized );
            }
        }

        // note that this will push an extra zero on the end of most channels. Zeroing of the accumulate function happens at initialization and at begin timestep
        channelDataMap.IncreaseChannelLength( name, 1 );
    }
}

bool
Report_Scenarios::notifyOnEvent(
    IIndividualHumanEventContext *context,
    const std::string& StateChange
)
{ 
    // get pointer to IIndividualHumanTB2 (full TB interface) and the person's SusceptibilityTB
    IIndividualHumanTB2 * individual_tb = nullptr;
    if( context->QueryInterface( GET_IID(IIndividualHumanTB2), (void**)&individual_tb ) != s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanTB2", "IIndividualHumanEventContext" );
    }
    release_assert( individual_tb );

    float mc_weight = (float)context->GetMonteCarloWeight();

    LOG_DEBUG_F( "Individual %d with weight %f experienced event %s\n",
                 context->GetSuid().data,
                 mc_weight,
                 StateChange.c_str()
               );

    if( StateChange == "TBActivation" )
    {
        Accumulate( _num_tbactivation_label, mc_weight );
    }
    else if( StateChange == "ProviderOrdersTBTest" )
    {
        Accumulate( _num_providerorderstbtest_label, mc_weight );
    }
    else if( StateChange == "TBTestDefault" )
    {
        Accumulate( _num_tbtestdefault_label, mc_weight );
    }
    else if( StateChange == "TBTestNegative" )
    {
        Accumulate( _num_tbtestnegative_label, mc_weight );
    }
    else if( StateChange == "TBTestPositive" )
    {
        Accumulate( _num_tbtestpositive_label, mc_weight );
    }
    else if( StateChange == "TBMDRTestDefault" )
    {
        Accumulate( _num_tbmdrtestdefault_label, mc_weight );
    }
    else if( StateChange == "TBMDRTestNegative" )
    {
        Accumulate( _num_tbmdrtestnegative_label, mc_weight );
    }
    else if( StateChange == "TBMDRTestPositive" )
    {
        Accumulate( _num_tbmdrtestpositive_label, mc_weight );
    }
    else if( StateChange == "TBStartDrugRegimen" )
    {
        Accumulate( _num_tbstartdrugregimen_label, mc_weight );
        if ( individual_tb->IsTreatmentNaive() ) //note the flag for m_is_tb_tx_naive_TBIVC = false is set after the broadcasting event, so that's why this works
        {
            Accumulate( _num_naive_tbstartdrugregimen_label, mc_weight );        
        }
        else
        {
            Accumulate( _num_retx_tbstartdrugregimen_label, mc_weight );
        }
    }
    else if( StateChange == "TBFailedDrugRegimen" )
    {
        Accumulate( _num_tbfaileddrugregimen_label, mc_weight );
    }
    else
    {
        LOG_DEBUG_F( "Un-handled event: %s\n", StateChange.c_str() );
    }
    return true;
}

// observers
BEGIN_QUERY_INTERFACE_BODY(Report_Scenarios)
    HANDLE_INTERFACE(IIndividualEventObserver)
END_QUERY_INTERFACE_BODY(Report_Scenarios)

}

