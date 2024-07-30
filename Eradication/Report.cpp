
#include "stdafx.h"

#include <functional>
#include <map>

#include "Report.h"
#include "INodeContext.h"
#include "IIndividualHuman.h"
#include "Interventions.h"
#include "Climate.h"
#include "NodeEventContext.h"
#include "IndividualEventContext.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "Report" )

namespace Kernel
{
static const std::string _report_name( "InsetChart.json" );

/////////////////////////
// Initialization methods
/////////////////////////

GET_SCHEMA_STATIC_WRAPPER_IMPL(Report,Report)

BEGIN_QUERY_INTERFACE_BODY( Report )
    HANDLE_INTERFACE( IIndividualEventObserver )
    HANDLE_ISUPPORTS_VIA( IReport )
END_QUERY_INTERFACE_BODY( Report )

Kernel::IReport*
Report::CreateReport()
{
    return new Report();
}

Report::Report()
: Report( _report_name )
{
}

Report::Report( const std::string& rReportName )
: BaseChannelReport( rReportName )
, m_InterventionNames()
, m_IPKeyNames()
, m_IPKeyValues()
, m_EventTriggerList()
, m_BroadcasterList()
, m_IsRegistered(false)
, new_infections(0.0f)
, new_reported_infections(0.0f)
, disease_deaths(0.0f)
, countOfSusceptibles(0.0f)
, countOfExposed(0.0f)
, countOfInfectious(0.0f)
, countOfRecovered(0.0f)
, countOfImmunized(0.0f)
, countOfWaning(0.0f)
, last_time( 0 )
, m_IncludePregnancies( false )
, m_PossibleMothers( 0.0f )
, m_NewPregnancies( 0.0f )
, m_Pregnant( 0.0f )
, stat_pop_id()
, new_infections_id()
, susceptible_pop_id()
, exposed_pop_id()
, infectious_pop_id()
, recovered_pop_id()
, waning_pop_id()
, hum_infectious_res_id()
, log_prev_id()
, births_id()
, infected_id()
, symtomatic_pop_id()
, newly_symptomatic_id()
, air_temp_id()
, land_temp_id()
, rainfall_id()
, relative_humidity_id()
, campaign_cost_id()
, disease_deaths_id()
, possible_mothers_id()
, new_pregnancies_id()
, pregnant_id()
, m_InterventionNameChannelIDs()
, m_IPKeyValuesChannelIDs()
{
    stat_pop_id           = AddChannel( "Statistical Population"     );
    new_infections_id     = AddChannel( "New Infections"             );
    hum_infectious_res_id = AddChannel( "Human Infectious Reservoir" );
    log_prev_id           = AddChannel( "Log Prevalence"             );
    births_id             = AddChannel( "Births"                     );
    infected_id           = AddChannel( "Infected"                   );
    symtomatic_pop_id     = AddChannel( "Symptomatic Population"     );
    newly_symptomatic_id  = AddChannel( "Newly Symptomatic"          );
    campaign_cost_id      = AddChannel( "Campaign Cost"              );
    disease_deaths_id     = AddChannel( "Disease Deaths"             );
}

bool Report::Configure( const Configuration* config )
{
    std::vector<std::string> tmp_names;
    initConfigTypeMap( GetParameterNameForHasInterventions(),
                       &tmp_names,
                       GetDescTextForHasInterventions(),
                       nullptr,
                       empty_set,
                       GetDependsOnForHasInterventions(),
                       nullptr );

    initConfigTypeMap( GetParameterNameForHasIP(),
                       &m_IPKeyNames,
                       GetDescTextForHasIP(),
                       nullptr,
                       empty_set,
                       GetDependsOnForHasIP(),
                       nullptr );

    initConfigTypeMap( GetParameterNameForIncludePregancies(),
                       &m_IncludePregnancies,
                       GetDescTextForIncludePregancies(),
                       false,
                       GetDependsOnForIncludePregancies(),
                       nullptr );

    bool is_configured = JsonConfigurable::Configure( config );
    if( is_configured && !JsonConfigurable::_dryrun )
    {
        for( auto name_str : tmp_names )
        {
            m_InterventionNames.push_back( InterventionName( name_str ) );
        }

        if( m_IncludePregnancies )
        {
            m_EventTriggerList.push_back( EventTrigger::Pregnant );
        }
    }
    return is_configured;
}

const char* Report::GetParameterNameForHasInterventions() const
{
    return "Inset_Chart_Has_Interventions";
}

const char* Report::GetDescTextForHasInterventions() const
{
    return Inset_Chart_Has_Interventions_DESC_TEXT;
}

const char* Report::GetDependsOnForHasInterventions() const
{
    return "Enable_Default_Reporting";
}

const char* Report::GetParameterNameForHasIP() const
{
    return "Inset_Chart_Has_IP";
}

const char* Report::GetDescTextForHasIP() const
{
    return Inset_Chart_Has_IP_DESC_TEXT;
}

const char* Report::GetDependsOnForHasIP() const
{
    return "Enable_Default_Reporting";
}

const char* Report::GetParameterNameForIncludePregancies() const
{
    return "Inset_Chart_Include_Pregnancies";
}

const char* Report::GetDescTextForIncludePregancies() const
{
    return Inset_Chart_Include_Pregnancies_DESC_TEXT;
}

const char* Report::GetDependsOnForIncludePregancies() const
{
    return "Enable_Default_Reporting";
}

/////////////////////////
// steady-state methods
/////////////////////////
void Report::Initialize( unsigned int nrmSize )
{
    AddSEIRChannels();

    for( auto iv_name : m_InterventionNames )
    {
        m_InterventionNameChannelIDs.push_back( AddChannel( std::string("Has_")+iv_name.ToString() ) );
    }

    if( m_IncludePregnancies )
    {
        possible_mothers_id = AddChannel( "Possible Mothers"   );
        new_pregnancies_id  = AddChannel( "New Pregnancies"    );
        pregnant_id         = AddChannel( "Currently Pregnant" );
    }

#ifndef DISABLE_CLIMATE
    if( ClimateFactory::climate_structure != ClimateStructure::CLIMATE_OFF )
    {
        air_temp_id          = AddChannel( "Air Temperature"   );
        land_temp_id         = AddChannel( "Land Temperature"  );
        rainfall_id          = AddChannel( "Rainfall"          );
        relative_humidity_id = AddChannel( "Relative Humidity" );
    }
#endif

    BaseChannelReport::Initialize( nrmSize );
    for( auto ipkey_name : m_IPKeyNames )
    {
        IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( ipkey_name );
        IPKeyValueContainer key_values = p_ip->GetValues<IPKeyValueContainer>();
        for( auto kv : key_values )
        {
            m_IPKeyValues.push_back( kv );
            m_IPKeyValuesChannelIDs.push_back( AddChannel( std::string("HasIP_")+kv.ToString() ) );
        }
    }
}

void Report::UpdateEventRegistration( float currentTime,
                                        float dt,
                                        std::vector<INodeEventContext*>& rNodeEventContextList,
                                        ISimulationEventContext* pSimEventContext )
{
    if( !m_IsRegistered )
    {
        for( auto pNEC : rNodeEventContextList )
        {
            IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();
            release_assert( broadcaster );
            m_BroadcasterList.push_back( broadcaster );

            for( auto trigger : m_EventTriggerList )
            {
                broadcaster->RegisterObserver( this, trigger );
            }
        }
        m_IsRegistered = true;
    }
}

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

    m_PossibleMothers   = 0.0f;
    m_Pregnant          = 0.0f;
    m_NewPregnancies    = 0.0f;

    for( auto& id : m_IPKeyValuesChannelIDs )
    {
        Accumulate( id, 0.0 );
    }
}

void Report::EndTimestep( float currentTime, float dt )
{
    Accumulate( disease_deaths_id, disease_deaths);

    if( m_IncludePregnancies )
    {
        Accumulate( possible_mothers_id, m_PossibleMothers );
        Accumulate( new_pregnancies_id,  m_NewPregnancies );
        Accumulate( pregnant_id,         m_Pregnant );
    }
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

    for( int i = 0; i < m_InterventionNames.size(); ++i )
    {
        const InterventionName& r_name = m_InterventionNames[ i ];
        const ChannelID&        r_id   = m_InterventionNameChannelIDs[ i ];
        float count = individual->GetInterventionsContext()->ContainsExistingByName( r_name ) ? monte_carlo_weight : 0.0;
        Accumulate( r_id, count );
    }

    IPKeyValueContainer* p_ind_kvs = individual->GetProperties();
    for( int i = 0; i < m_IPKeyValues.size(); ++i )
    {
        if( p_ind_kvs->Contains( m_IPKeyValues[ i ]) )
        {
            Accumulate( m_IPKeyValuesChannelIDs[ i ], monte_carlo_weight);
        }
    }

    if( m_IncludePregnancies )
    {
        if( individual->IsPregnant() )
            m_Pregnant += monte_carlo_weight;

        if( individual->IsPossibleMother() )
            m_PossibleMothers += monte_carlo_weight;
    }
}

void
Report::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    LOG_DEBUG( "LogNodeData\n" );

    Accumulate( stat_pop_id,          pNC->GetStatPop() );
    Accumulate( births_id,            pNC->GetBirths() );
    Accumulate( infected_id,          pNC->GetInfected() );
    Accumulate( symtomatic_pop_id,    pNC->GetSymptomatic() );
    Accumulate( newly_symptomatic_id, pNC->GetNewlySymptomatic() + new_reported_infections ); //either GetNewlySymptomatic() or new_reported_infections is used (other channel is 0)
    new_reported_infections = 0.0f;

    if (pNC->GetLocalWeather())
    {
        Accumulate( air_temp_id,          pNC->GetLocalWeather()->airtemperature() );
        Accumulate( land_temp_id,         pNC->GetLocalWeather()->landtemperature() );
        Accumulate( rainfall_id,          pNC->GetLocalWeather()->accumulated_rainfall() );
        Accumulate( relative_humidity_id, pNC->GetLocalWeather()->humidity());
    }

    Accumulate( new_infections_id, new_infections );
    new_infections = 0.0f;

    Accumulate( campaign_cost_id,      pNC->GetCampaignCost() );
    Accumulate( hum_infectious_res_id, pNC->GetInfectivity() );

    AccumulateSEIRW();
}

bool Report::notifyOnEvent( IIndividualHumanEventContext *context,
                            const EventTrigger& trigger )
{
    if( m_IncludePregnancies && (trigger == EventTrigger::Pregnant) )
    {
        m_NewPregnancies += context->GetMonteCarloWeight();
    }

    return true;
}

void 
Report::populateSummaryDataUnitsMap(
    std::map<std::string, std::string> &units_map
)
{
    units_map[ stat_pop_id.GetName()           ] = "Population";
    units_map[ births_id.GetName()             ] = "Births";
    units_map[ infected_id.GetName()           ] = infected_id.GetName() ;
    units_map[ log_prev_id.GetName()           ] = "Log Prevalence";
    units_map[ new_infections_id.GetName()     ] = "";
    units_map[ disease_deaths_id.GetName()     ] = "";
    units_map[ campaign_cost_id.GetName()      ] = "USD";
    units_map[ hum_infectious_res_id.GetName() ] = "Infectivity/Person";

    if( m_IncludePregnancies )
    {
        units_map[ possible_mothers_id.GetName() ] = "NumWomen";
        units_map[ new_pregnancies_id.GetName()  ] = "NumWomen";
        units_map[ pregnant_id.GetName()         ] = "NumWomen";
    }

#ifndef DISABLE_CLIMATE
    if( ClimateFactory::climate_structure != ClimateStructure::CLIMATE_OFF )
    {
        units_map[ rainfall_id.GetName() ] = "mm/day";
        units_map[ air_temp_id.GetName() ] = "degrees C";
    }
#endif

    for( auto& id : m_InterventionNameChannelIDs )
    {
        units_map[ id.GetName() ] = "Fraction of Population";
    }
    for( auto id : m_IPKeyValuesChannelIDs )
    {
        units_map[ id.GetName()] = "Fraction of Population";
    }

    AddSEIRWUnits(units_map);
}


// normalize by timestep and create derived channels
void
Report::postProcessAccumulatedData()
{
    LOG_DEBUG( "postProcessAccumulatedData\n" );

    normalizeChannel( infected_id.GetName(), stat_pop_id.GetName() );

#ifndef DISABLE_CLIMATE
    if( ClimateFactory::climate_structure != ClimateStructure::CLIMATE_OFF )
    {
        normalizeChannel( air_temp_id.GetName(),          float(_nrmSize) );
        normalizeChannel( land_temp_id.GetName(),         float(_nrmSize) );
        normalizeChannel( relative_humidity_id.GetName(), float(_nrmSize) );
        normalizeChannel( rainfall_id.GetName(),          float(_nrmSize) * (1 / 1000.0f) ); // multiply by 1000 to get result in mm/day
    }
#endif

    normalizeChannel( hum_infectious_res_id.GetName(), stat_pop_id.GetName() );

    // add derived channels
    addDerivedLogScaleSummaryChannel( infected_id.GetName(), log_prev_id.GetName() );

    for( auto& id : m_InterventionNameChannelIDs )
    {
        normalizeChannel( id.GetName(), stat_pop_id.GetName() );
    }
    for( auto& id : m_IPKeyValuesChannelIDs )
    {
        normalizeChannel( id.GetName(), stat_pop_id.GetName() );
    }
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

void Report::AddSEIRChannels()
{
    susceptible_pop_id = AddChannel( "Susceptible Population");
    exposed_pop_id     = AddChannel( "Exposed Population"    );
    infectious_pop_id  = AddChannel( "Infectious Population" );
    recovered_pop_id   = AddChannel( "Recovered Population"  );
    waning_pop_id      = AddChannel( "Waning Population"     );
}

void Report::AccumulateSEIRW()
{
    Accumulate( susceptible_pop_id, countOfSusceptibles );
    Accumulate( exposed_pop_id,     countOfExposed      );
    Accumulate( infectious_pop_id,  countOfInfectious   );
    Accumulate( recovered_pop_id,   countOfRecovered    );
    Accumulate( waning_pop_id,      countOfWaning       );

    countOfSusceptibles = 0.0f;
    countOfExposed      = 0.0f;
    countOfInfectious   = 0.0f;
    countOfRecovered    = 0.0f;
    countOfWaning       = 0.0f;
}

void Report::AddSEIRWUnits( std::map<std::string, std::string> &units_map )
{
    units_map[ susceptible_pop_id.GetName() ] = "Susceptible Fraction";
    units_map[ exposed_pop_id.GetName() ]     = "Exposed Fraction";
    units_map[ infectious_pop_id.GetName() ]  = "Infectious Fraction";
    units_map[ recovered_pop_id.GetName() ]   = "Recovered (Immune) Fraction";
    units_map[ waning_pop_id.GetName() ]      = "Waning Immunity Fraction";
}

void Report::NormalizeSEIRWChannels()
{
    normalizeChannel( susceptible_pop_id.GetName(), stat_pop_id.GetName() );
    normalizeChannel( exposed_pop_id.GetName(),     stat_pop_id.GetName() );
    normalizeChannel( infectious_pop_id.GetName(),  stat_pop_id.GetName() );
    normalizeChannel( recovered_pop_id.GetName(),   stat_pop_id.GetName() );
    normalizeChannel( waning_pop_id.GetName(),      stat_pop_id.GetName() );
}

void Report::Reduce()
{
    BaseChannelReport::Reduce();

    // make sure we are unregistered before objects start being deleted
    for( auto broadcaster : m_BroadcasterList )
    {
        for( auto trigger : m_EventTriggerList )
        {
            broadcaster->UnregisterObserver( this, trigger );
        }
    }
    m_BroadcasterList.clear();
}

}
