
#include "stdafx.h"

#include <map>
#include "Debug.h"
#include "DemographicsReport.h"
#include "INodeContext.h"
#include "Exceptions.h"
#include "IIndividualHuman.h"
#include "SimulationEnums.h"

using namespace std;
using namespace json;
using namespace Kernel;

SETUP_LOGGING( "DemographicsReport" )

static const std::string _report_name = "DemographicsSummary.json";

// age-buckets are every 5 years, mirroring census-type data 
// (for example, see http://www.census.gov/population/international/data/idb/informationGateway.php)
float DemographicsReport::age_buckets[] = {  1825.0f,  3650.0f,  5475.0f,  7300.0f,  9125.0f,
                                            10950.0f, 12775.0f, 14600.0f, 16425.0f, 18250.0f,
                                            20075.0f, 21900.0f, 23725.0f, 25550.0f, 27375.0f,
                                            29200.0f, 31025.0f, 32850.0f, 34675.0f, 36500.0f, FLT_MAX };

std::string DemographicsReport::age_ranges[] = {    "<5",   "5-9", "10-14", "15-19", "20-24", "25-29", "30-34",
                                                 "35-39", "40-44", "45-49", "50-54", "55-59", "60-64", "65-69", 
                                                 "70-74", "75-79", "80-84", "85-89", "90-94", "95-99", ">100" };

Kernel::IReport*
DemographicsReport::CreateReport()
{
    return new DemographicsReport();
}

DemographicsReport::DemographicsReport()
: BaseChannelReport( _report_name )
, pseudoPop(0)
, totalAge(0.0f)
, totalMales(0.0f)
, oldBirths(0.0f)
, totalBirths(0.0f)
, newNaturalDeaths(0.0f)
, avg_age_id()
, gender_id()
, pseudo_pop_id()
, births_id()
, deaths_id()
, stat_pop_id()
, mothers_id()
, pop_age_ids()
{
    avg_age_id    = AddChannel( "Average Age"                  );
    gender_id     = AddChannel( "Gender Ratio (fraction male)" );
    pseudo_pop_id = AddChannel( "Pseudo-Population"            );
    births_id     = AddChannel( "New Births"                   );
    deaths_id     = AddChannel( "New Natural Deaths"           );
    stat_pop_id   = AddChannel( "Statistical Population"       );
    mothers_id    = AddChannel( "Possible Mothers"             );

    int num_age_buckets = sizeof(age_buckets) / sizeof(float);
    for(int age_bucket = 0; age_bucket < num_age_buckets; age_bucket++) // including <5 and >100 in this loop
    {
        std::string pop_str = "Population Age " + age_ranges[age_bucket];
        ChannelID id = AddChannel( pop_str );
        pop_age_ids.push_back( id );
    }
}

void DemographicsReport::Initialize( unsigned int nrmSize )
{
    _nrmSize = nrmSize;
    release_assert( _nrmSize );
}

void DemographicsReport::BeginTimestep()
{
    BaseChannelReport::BeginTimestep();

    pseudoPop = 0;

    totalMales = 0.0f;
    totalAge = 0.0f;
    newNaturalDeaths = 0.0f;

    oldBirths = totalBirths;
    totalBirths = 0.0f;

    for(int age_bucket = 0; age_bucket < pop_age_ids.size(); age_bucket++ )
        population_by_age_bucket[age_bucket] = 0.0f;
}

void DemographicsReport::EndTimestep( float currentTime, float dt )
{
    // these 2 channels get normalized (divided by total statPop across all nodes) later...
    Accumulate( avg_age_id, totalAge );
    Accumulate( gender_id, totalMales );

    Accumulate( pseudo_pop_id, (float)pseudoPop );

    for( int i = 0; i < pop_age_ids.size(); ++i )
    {
        Accumulate( pop_age_ids[ i ], population_by_age_bucket[ i ]);
    }

    Accumulate( births_id, totalBirths - oldBirths );
    Accumulate( deaths_id, newNaturalDeaths );
}

void
DemographicsReport::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    Accumulate(stat_pop_id, pNC->GetStatPop());
    Accumulate(mothers_id, (float)pNC->GetPossibleMothers());

    totalBirths += pNC->GetBirths();
}

void
DemographicsReport::LogIndividualData(
    Kernel::IIndividualHuman* individual
)
{
    pseudoPop++;

    float age = (float)individual->GetAge();
    float monte_carlo_weight = (float)individual->GetMonteCarloWeight();

    totalAge += age * monte_carlo_weight;

    if(individual->GetGender() == Gender::MALE)
        totalMales += monte_carlo_weight;

    if(individual->GetStateChange() == HumanStateChange::DiedFromNaturalCauses)
        newNaturalDeaths += monte_carlo_weight;

    for( int age_bucket = 0; age_bucket < pop_age_ids.size(); ++age_bucket )
    {
        if( age < age_buckets[ age_bucket ] )
        {
            population_by_age_bucket[age_bucket] += monte_carlo_weight;
            break;
        }
    }
}


void DemographicsReport::postProcessAccumulatedData()
{
    normalizeChannel( avg_age_id.GetName(), stat_pop_id.GetName() );
    normalizeChannel( gender_id.GetName(),  stat_pop_id.GetName() );
}
