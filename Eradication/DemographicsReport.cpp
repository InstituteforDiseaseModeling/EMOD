/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <map>
#include "BoostLibWrapper.h"
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
                                            29200.0f, 31025.0f, 32850.0f, 34675.0f, 36500.0f };

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
{
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

    int num_age_buckets = sizeof(population_by_age_bucket) / sizeof(float);
    for(int age_bucket = 0; age_bucket < num_age_buckets; age_bucket++)
        population_by_age_bucket[age_bucket] = 0.0f;
}

void DemographicsReport::EndTimestep( float currentTime, float dt )
{
    // these 2 channels get normalized (divided by total statPop across all nodes) later...
    Accumulate("Average Age", totalAge);
    Accumulate("Gender Ratio (fraction male)", totalMales);

    Accumulate("Pseudo-Population", (float)pseudoPop);

    int num_age_buckets = sizeof(age_buckets) / sizeof(float);
    for(int age_bucket = 0; age_bucket <= num_age_buckets; age_bucket++) // including <5 and >100 in this loop
    {
        std::string pop_str = "Population Age " + age_ranges[age_bucket];
        Accumulate( pop_str, population_by_age_bucket[age_bucket] );
    }

    Accumulate("New Births", totalBirths - oldBirths);
    Accumulate("New Natural Deaths", newNaturalDeaths);
}

void
DemographicsReport::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    Accumulate("Statistical Population", pNC->GetStatPop());
    Accumulate("Possible Mothers", (float)pNC->GetPossibleMothers());

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

    int num_age_buckets = sizeof(age_buckets) / sizeof(float);
    int age_bucket;

    for(age_bucket = 0; age_bucket < num_age_buckets; age_bucket++)
        if(age < age_buckets[age_bucket])  break;

    population_by_age_bucket[age_bucket] += monte_carlo_weight;
}


void DemographicsReport::postProcessAccumulatedData()
{
    normalizeChannel("Average Age",                  "Statistical Population");
    normalizeChannel("Gender Ratio (fraction male)", "Statistical Population");
}
