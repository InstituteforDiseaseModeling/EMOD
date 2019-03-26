/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "IdmString.h"
#include "IIndividualHuman.h"
#include "PropertyReport.h"
#include "Properties.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "PropertyReport" )

static const std::string _report_name = "PropertyReport.json";

namespace Kernel {

const char * PropertyReport::_pop_label = "Statistical Population:";
const char * PropertyReport::_infected_label = "Infected:";
const char * PropertyReport::_new_infections_label = "New Infections:";
const char * PropertyReport::_disease_deaths_label = "Disease Deaths:";

/////////////////////////
// Initialization methods
/////////////////////////
IReport*
PropertyReport::CreateReport()
{
    return new PropertyReport( _report_name );
}

PropertyReport::PropertyReport( const std::string& rReportName )
: Report( rReportName )
, permutationsSet()
, permutationsList()
, new_infections()
, new_reported_infections()
, disease_deaths()
, infected()
, statPop()
{
}

/////////////////////////
// steady-state methods
/////////////////////////

template<typename T>
static std::set< std::string > getKeys( const T &propMap )
{
    // put all keys in set
    std::set< std::string > allKeys;
    for (auto& entry : propMap)
    {
        allKeys.insert( entry.first );
    }
    return allKeys;
}

void
PropertyReport::GenerateAllPermutationsOnce(
    std::set< std::string > keyskeys,
    tKeyValuePairs perm,
    tPermutations& permsSet
)
{
    if( keyskeys.size() )
    {
        const std::string key = *keyskeys.begin();
        keyskeys.erase( key );
        const IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( key );
        for( auto kv : p_ip->GetValues<IPKeyValueContainer>() )
        {
            std::string value = kv.GetValueAsString();
            auto kvp = perm;
            kvp.insert( make_pair( key, value ) );
            GenerateAllPermutationsOnce( keyskeys, kvp, permsSet );
        }
    }
    else
    {
        permsSet.insert( perm );
    }
}

void
PropertyReport::LogIndividualData(
    Kernel::IIndividualHuman* individual
)
{
    std::string reportingBucket = individual->GetPropertyReportString();
    if( reportingBucket.empty() ) // call this just first time.
    {
        auto permKeys = IPFactory::GetInstance()->GetKeysAsStringSet();
        if( permutationsSet.size() == 0 )
        {
            // put all keys in set
            tKeyValuePairs actualPerm;
            // Commented out is what we WANT to do but it behaves oddly. With more time I'm sure we can fix it.
            //IPFactory::GetInstance()->GenerateAllPermutationsOnce( permKeys, actualPerm, permutationsSet ); 
            GenerateAllPermutationsOnce( permKeys, actualPerm, permutationsSet );
        }

        tProperties prop_map = individual->GetProperties()->GetOldVersion();
        if( prop_map.size() == 0 )
        {
            LOG_WARN_F( "Individual %lu aged %f (years) had no properties in %s.\n", individual->GetSuid().data, individual->GetAge()/DAYSPERYEAR, __FUNCTION__ );
            // This seems to be people reaching "old age" (i.e., over 125)
            return;
        }

        // Copy all property keys from src to dest but only if present in permKeys
        auto src = getKeys( prop_map );

        // copy-if setup
        std::vector<std::string> dest( src.size() );
        auto it = std::copy_if (src.begin(), src.end(), dest.begin(), [&](std::string test)
            {
                return ( std::find( permKeys.begin(), permKeys.end(), test ) != permKeys.end() );
            }
        );
        dest.resize(std::distance(dest.begin(),it));

        // new map from those keys that make it through filter
        tProperties permProps;
        for( auto &entry : dest )
        {
            permProps.insert( std::make_pair( entry, prop_map.at(entry) ) );
        }
        // Try an optimized solution that constructs a reporting bucket string based entirely
        // on the properties of the individual. But we need some rules. Let's start with simple
        // alphabetical ordering of category names
        reportingBucket = PropertiesToString( permProps );
        individual->SetPropertyReportString( reportingBucket );
    }

    float monte_carlo_weight = (float)individual->GetMonteCarloWeight();
    NewInfectionState::_enum nis = individual->GetNewInfectionState();

    if(nis == NewInfectionState::NewAndDetected || nis == NewInfectionState::NewInfection)
        new_infections[ reportingBucket ] += monte_carlo_weight;

    if(nis == NewInfectionState::NewAndDetected || nis == NewInfectionState::NewlyDetected)
        new_reported_infections[ reportingBucket ] += monte_carlo_weight;

    if(individual->GetStateChange() == HumanStateChange::KilledByInfection)
        disease_deaths[ reportingBucket ] += monte_carlo_weight;

    if (individual->IsInfected())
    {
        infected[ reportingBucket ] += monte_carlo_weight;
    }

    statPop[ reportingBucket ] += monte_carlo_weight;
}

void
PropertyReport::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    LOG_DEBUG( "LogNodeData\n" );
    for (auto& kvp : permutationsSet)
    {
        std::string reportingBucket = PropertiesToString( kvp );

        Accumulate(_new_infections_label + reportingBucket, new_infections[reportingBucket]);
        new_infections[reportingBucket] = 0.0f;
        //Accumulate(_disease_deaths_label + reportingBucket, disease_deaths[reportingBucket]);
        Accumulate(_pop_label + reportingBucket, statPop[ reportingBucket ] );
        statPop[reportingBucket] = 0.0f;
        Accumulate(_infected_label + reportingBucket, infected[ reportingBucket ]);
        infected[ reportingBucket ] = 0.0f;
    }
}

// normalize by time step and create derived channels
void
PropertyReport::postProcessAccumulatedData()
{
    LOG_DEBUG( "postProcessAccumulatedData in PropertyReport\n" );

}

// This is just to avoid Disease Deaths not broken down by props which is in base class. Why is it there?
void
PropertyReport::EndTimestep( float currentTime, float dt )
{
    LOG_DEBUG( "EndTimestep\n" );
}

};
