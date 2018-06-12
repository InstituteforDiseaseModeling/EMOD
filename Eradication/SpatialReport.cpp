/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <functional>
#include <map>
#include <vector>
#include "BoostLibWrapper.h"
// not in boost wrapper???
#include <boost/math/special_functions/fpclassify.hpp>

#include "SpatialReport.h"
#include "INodeContext.h"
#include "Climate.h"
#include "Sugar.h"
#include "Debug.h"
#include "Environment.h"
#include "FileSystem.h"
#include "Exceptions.h"
#include "IIndividualHuman.h"
#include "ProgVersion.h"
#include "IdmMpi.h"

using namespace std;

SETUP_LOGGING( "SpatialReport" )

static const string _report_name = "SpatialReport"; // is this what it should be called...?  multiple files...

namespace Kernel {

    BEGIN_QUERY_INTERFACE_BODY(SpatialReport)
    END_QUERY_INTERFACE_BODY(SpatialReport)
    GET_SCHEMA_STATIC_WRAPPER_IMPL(SpatialReport,SpatialReport)

/////////////////////////
// Initialization methods
/////////////////////////
IReport*
SpatialReport::CreateReport()
{
    return new SpatialReport();
}

SpatialReport::SpatialReport()
: BaseChannelReport( _report_name )
, air_temperature_info(             "Air_Temperature",                  "degrees C")
, births_info(                      "Births",                           "")
, campaign_cost_info(               "Campaign_Cost",                    "US dollars")
, disease_deaths_info(              "Disease_Deaths",                   "")
, human_infectious_reservoir_info(  "Human_Infectious_Reservoir",       "???")
, infection_rate_info(              "Infection_Rate",                   "???")
, land_temperature_info(            "Land_Temperature",                 "degrees C")
, new_infections_info(              "New_Infections",                   "")
, new_reported_infections_info(     "New_Reported_Infections",          "")
, population_info(                  "Population",                       "")
, prevalence_info(                  "Prevalence",                       "infected fraction")
, rainfall_info(                    "Rainfall",                         "mm")
, relative_humidity_info(           "Relative_Humidity",                "fraction")
, new_infections(0.0f)
, new_reported_infections(0.0f)
, disease_deaths(0.0f)
, nodeid_index_map()
, has_shuffled_nodes(false)
, total_timesteps(0)
, channel_file_map()
, spatial_output_channels()
{
}

void SpatialReport::populateChannelInfos(tChanInfoMap &channel_infos)
{
    channel_infos[ air_temperature_info.name ] = &air_temperature_info;
    channel_infos[ births_info.name ] = &births_info;
    channel_infos[ campaign_cost_info.name ] = &campaign_cost_info;
    channel_infos[ disease_deaths_info.name ] = &disease_deaths_info;
    channel_infos[ human_infectious_reservoir_info.name ] = &human_infectious_reservoir_info;
    channel_infos[ infection_rate_info.name ] = &infection_rate_info;
    channel_infos[ land_temperature_info.name ] = &land_temperature_info;
    channel_infos[ new_infections_info.name ] = &new_infections_info;
    channel_infos[ new_reported_infections_info.name ] = &new_reported_infections_info;
    channel_infos[ population_info.name ] = &population_info;
    channel_infos[ prevalence_info.name ] = &prevalence_info;
    channel_infos[ rainfall_info.name ] = &rainfall_info;
    channel_infos[ relative_humidity_info.name ] = &relative_humidity_info;
}

void
SpatialReport::Initialize( unsigned int nrmSize )
{
    _nrmSize = nrmSize;
    release_assert( _nrmSize );

    // compile information about supported channels
    tChanInfoMap channel_infos;
    populateChannelInfos(channel_infos);

    // read channels to enable from config
    for (auto& channel_name : spatial_output_channels)
    {
        if( channel_infos.find( channel_name ) == channel_infos.end() )
        {
            ostringstream oss;
            oss << "Spatial_Output_Channels value \"" << channel_name << "\"";
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "of potential spatial-output channel names", oss.str().c_str());
        }
        channel_infos[ channel_name ]->enabled = true;

        channelDataMap.IncreaseChannelLength( channel_name, _nrmSize );
    }

    channelDataMap.IncreaseChannelLength( "NodeID", _nrmSize );

    if( !channelDataMap.HasChannel("Population") )
    {
        population_info.enabled = true ;
        channelDataMap.IncreaseChannelLength( "Population", _nrmSize );
    }

    total_timesteps = 0;
}

bool SpatialReport::Configure(
    const Configuration* config
)
{
    tChanInfoMap channel_infos;
    populateChannelInfos(channel_infos);
    spatial_output_channels.possible_values = getKeys( channel_infos );
    initConfigTypeMap( "Spatial_Output_Channels", &spatial_output_channels, Spatial_Output_Channels_DESC_TEXT);
    return JsonConfigurable::Configure( config );
}


/////////////////////////
// steady-state methods
/////////////////////////
void SpatialReport::BeginTimestep()
{
    // we increase the length in Initialize() so we don't want to do it on the first time through
    if( has_shuffled_nodes )
    {
        channelDataMap.IncreaseChannelLength( _nrmSize );
    }
}

void
SpatialReport::LogIndividualData(
    Kernel::IIndividualHuman* individual
)
{
    LOG_DEBUG( "LogIndividualData\n" );

    float monte_carlo_weight = (float)individual->GetMonteCarloWeight();

    NewInfectionState::_enum nis = individual->GetNewInfectionState();

    if(nis == NewInfectionState::NewAndDetected || nis == NewInfectionState::NewInfection)
        new_infections += monte_carlo_weight;

    if(nis == NewInfectionState::NewAndDetected || nis == NewInfectionState::NewlyDetected)
        new_reported_infections += monte_carlo_weight;

    if(individual->GetStateChange() == HumanStateChange::KilledByInfection)
        disease_deaths += monte_carlo_weight;
}

void
SpatialReport::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    LOG_DEBUG( "LogNodeData\n" );

    // a bit of a hack... store nodeid's as "floats" so that we can just reduce them like any other channel;
    // they're same size as floats anyway, so we can write them out to the output file just fine...
    auto nodeid = pNC->GetExternalID();
    float * flt = (float*)&nodeid;
    Accumulate("NodeID", nodeid, *flt);

    if(new_infections_info.enabled)
    {
        Accumulate(new_infections_info.name, nodeid, new_infections);
        new_infections = 0.0f;
    }

    if(new_reported_infections_info.enabled)
    {
        Accumulate(new_reported_infections_info.name, nodeid, new_reported_infections);
        new_reported_infections = 0.0f;
    }

    // always accumulate stat-pop, because it frequently gets used for normalization of other channels
    Accumulate(population_info.name, nodeid, pNC->GetStatPop());

    if(births_info.enabled)
        Accumulate(births_info.name, nodeid, pNC->GetBirths());

    if(prevalence_info.enabled)
        Accumulate(prevalence_info.name, nodeid, pNC->GetInfected());

    const Kernel::Climate* weather = pNC->GetLocalWeather();
    if(weather)
    {
        if(air_temperature_info.enabled)
            Accumulate(air_temperature_info.name, nodeid, weather->airtemperature());
        if(land_temperature_info.enabled)
            Accumulate(land_temperature_info.name, nodeid, weather->landtemperature());
        if(rainfall_info.enabled)
            Accumulate(rainfall_info.name, nodeid, weather->accumulated_rainfall());
        if(relative_humidity_info.enabled)
            Accumulate(relative_humidity_info.name, nodeid, weather->humidity());
    }

    if(disease_deaths_info.enabled)
    {
        Accumulate(disease_deaths_info.name, nodeid, disease_deaths);
        disease_deaths = 0.0f;
    }

    if(campaign_cost_info.enabled)
        Accumulate(campaign_cost_info.name, nodeid, pNC->GetCampaignCost());

    if(human_infectious_reservoir_info.enabled)
        Accumulate(human_infectious_reservoir_info.name, nodeid, pNC->GetInfectivity());

    if(infection_rate_info.enabled)
        Accumulate(infection_rate_info.name, nodeid, pNC->GetInfectionRate());
}

void SpatialReport::EndTimestep( float currentTime, float dt )
{
    // TODO: need to take care of end of simulation if you're not writing every timestep...

    if( !has_shuffled_nodes )
    {
        has_shuffled_nodes = true;
        shuffleNodeData();
    }

    Reduce();

    // if rank 0, write out the files
    if(EnvPtr->MPI.Rank == 0)
    {
        postProcessAccumulatedData();

        InitializeFiles();

        // -------------------------------------------------------
        // --- Passing a member variable to the function so that 
        // --- subclasses can pass 'custom' maps if they want to.
        // -------------------------------------------------------
        WriteData( channelDataMap );
    }
    ClearData();
}

void SpatialReport::InitializeFiles()
{
    std::vector<std::string> channel_names = channelDataMap.GetChannelNames();

    if( channel_file_map.size() == 0 )
    {
        for( auto name : channel_names )
        {
            // TODO: should be checking if there's a ChannelInfo enabled instead...?
            if( spatial_output_channels.count( name ) <= 0 )
                continue;

            string filepath = FileSystem::Concat( EnvPtr->OutputPath, (report_name + "_" + name + ".bin") );
            ofstream* file = new ofstream();
            FileSystem::OpenFileForWriting( *file, filepath.c_str(), true );

            channel_file_map[ name ] = file;

            WriteHeader( file );
        }
    }
}

void SpatialReport::WriteData( ChannelDataMap& rChannelDataMap )
{
    total_timesteps++;

    std::vector<std::string> channel_names = rChannelDataMap.GetChannelNames();

    for( auto name : channel_names )
    {
        // TODO: should we be checking if there's a ChannelInfo enabled instead...?
        if( spatial_output_channels.count( name ) <= 0 )
            continue;

        LOG_DEBUG_F( "Writing out spatial output for channel %s\n", name.c_str() );
        const ChannelDataMap::channel_data_t& r_channel_data = rChannelDataMap.GetChannel( name );
        channel_file_map[ name ]->write( (char*)(&r_channel_data[ 0 ]), r_channel_data.size() * sizeof( ChannelDataMap::channel_data_element_t ) );
    }
}

void SpatialReport::ClearData()
{
    // clear already written data
    channelDataMap.ClearData();
}

void SpatialReport::WriteHeader( std::ofstream* file )
{
    WriteHeaderParameters( file );

    const ChannelDataMap::channel_data_t& r_channel_data = channelDataMap.GetChannel( "NodeID" );

    file->write( (char*)&r_channel_data[ 0 ], _nrmSize * sizeof( int ) );
    //file->write((char*)&channelDataMap["NodeID"][0], _nrmSize * sizeof(int));
}

void SpatialReport::WriteHeaderParameters( std::ofstream* file )
{
    int neg_one = -1;
    file->write( (char*)&_nrmSize, sizeof( int ) );
    file->write( (char*)&neg_one, sizeof( int ) ); // placeholder for # of timesteps later when we know how many there are
}

/////////////////////////
// Finalization methods
/////////////////////////

void
SpatialReport::Finalize()
{
    // now that we know how many timesteps there were, go back and fill that in at the beginning
    // of the output files

    for (auto& entry : channel_file_map)
    {
        ofstream* file = entry.second;

        LOG_DEBUG_F("Finalizing spatial-report channel \"%s\"\n", entry.first.c_str());

        file->seekp(sizeof(int));
        file->write((char*)&total_timesteps, sizeof(int));

        file->close();
    }

    // write a header?  consensus is no at this point, but we may want to add it in the future...

}


void
SpatialReport::Accumulate( std::string channel_name, int nodeid, float value )
{
    std::map<int,int>::iterator it = nodeid_index_map.find(nodeid);

    if(it == nodeid_index_map.end())
    {
        nodeid_index_map[nodeid] = nodeid_index_map.size();
    }

    int node_index = channelDataMap.GetChannelLength() - _nrmSize + nodeid_index_map[nodeid];

    channelDataMap.Accumulate( channel_name, node_index, value );
}

void 
SpatialReport::populateSummaryDataUnitsMap(
    map<string, string> &units_map
)
{
    // not needed?
}

void
SpatialReport::postProcessAccumulatedData()
{
    LOG_DEBUG( "postProcessAccumulatedData\n" );

    if ( prevalence_info.enabled )
    {
        if ( population_info.enabled )
        {
            normalizeChannel(prevalence_info.name, population_info.name);
        }
        else
        {
            throw GeneralConfigurationException(  __FILE__, __LINE__, __FUNCTION__, "If 'Prevalence' is enabled, then 'Population' must be enabled.");
        }
    }

    if ( rainfall_info.enabled )
    {
        normalizeChannel(rainfall_info.name, (1 / 1000.0f)); // multiply by 1000 (divide by 1/1000) to get result in mm
    }

    // Turn these off for now... can add them back later if they're really needed
    //addDerivedCumulativeSummaryChannel(new_infections_info.name, "Cumulative_Infections");
    //addDerivedCumulativeSummaryChannel(new_reported_infections_info.name, "Cumulative_Reported_Infections");
}

// override this because we need to skip every nrmSize entries when doing the sum...
void SpatialReport::addDerivedCumulativeSummaryChannel(string channel_name, string channel_cumulative_name)
{
    LOG_DEBUG( "addDerivedCumulativeSummaryChannel\n" );

    static map<string, vector<float> > channel_sums;

    int channel_size = channelDataMap.GetChannelLength();

    if (channel_size > 0)
    {
        if(channel_sums.count(channel_name) == 0)
        {
            channel_sums[channel_name] = vector<float>(_nrmSize);
            channelDataMap.IncreaseChannelLength( channel_cumulative_name, channel_size );
        }

        vector<float> &sums = channel_sums[channel_cumulative_name];
        const ChannelDataMap::channel_data_t &channel_data = channelDataMap.GetChannel( channel_name );

        for (int k = 0; k < channel_size; k++)
        {
            sums[k % _nrmSize] += channel_data[k];
            channelDataMap.Accumulate( channel_cumulative_name, k, sums[k % _nrmSize] );
        }
    }
}

void SpatialReport::shuffleNodeData()
{
    vector<int> nodeids;
    vector<int> all_nodeids;
    for (auto& entry : nodeid_index_map)
    {
        nodeids.push_back(entry.first);
    }

    // synchronize nodeids and sort
    EnvPtr->MPI.p_idm_mpi->Sync( nodeids, all_nodeids );

    std::sort(all_nodeids.begin(), all_nodeids.end());

    // figure out the correct indices for nodes processed by this rank
    std::map<int, int> new_nodeid_index_map;
    for(int i = 0; i < all_nodeids.size(); i++)
    {
        if(nodeid_index_map.find(all_nodeids[i]) != nodeid_index_map.end())
            new_nodeid_index_map[all_nodeids[i]] = i;
    }

    // swap the data around for all channels so they're stored in the correct indices (according
    // to nodeid)
    std::vector<std::string> channel_names = channelDataMap.GetChannelNames();
    for( auto name : channel_names )
    {
        ChannelDataMap::channel_data_t new_channel_data(_nrmSize);
        const ChannelDataMap::channel_data_t& old_channel_data = channelDataMap.GetChannel( name );

        for(int i = 0; i < nodeids.size(); i++)
            new_channel_data[new_nodeid_index_map[nodeids[i]]] = old_channel_data[nodeid_index_map[nodeids[i]]];

        channelDataMap.SetChannelData( name, new_channel_data );
    }

    nodeid_index_map = new_nodeid_index_map;
}

}
