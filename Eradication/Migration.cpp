/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include <cstdlib>
#include <stdio.h>
#include <strstream>
#include "Migration.h"
#include "Debug.h"
#include "Contexts.h"
#include "Node.h"
#include "RANDOM.h"
#include "Environment.h"
#include "FileSystem.h"
#include "NodeDemographics.h"
#include "Common.h"
#include "Exceptions.h"
#include "BoostLibWrapper.h"
#include <vector>
#include "suids.hpp"
#include "SimulationConfig.h"

using namespace std;
using namespace json;

#pragma warning(disable: 4244)
#pragma warning(disable: 4305)
// 4503: decorated name length exceeded, boost recommends suppression
#pragma warning(disable: 4503)

static const char * _module = "Migration";

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Migration.General,MigrationInfoFactory)
    BEGIN_QUERY_INTERFACE_BODY(MigrationInfoFactory)
    END_QUERY_INTERFACE_BODY(MigrationInfoFactory)
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Migration,MigrationInfo)
    BEGIN_QUERY_INTERFACE_BODY(MigrationInfo)
    END_QUERY_INTERFACE_BODY(MigrationInfo)


    MigrationInfoFactory::MigrationInfoFactory()
    : JsonConfigurable()
    , airmig_filename()
    , localmig_filename()
    , regionmig_filename()
    , seamig_filename()
    , airplane_migration(false)
    , regional_migration(false)
    , sea_migration(false)
    , local_migration(false)
    , local_migration_file(nullptr)
    , air_migration_file(nullptr)
    , regional_migration_file(nullptr)
    , sea_migration_file(nullptr)
    , local_migration_offsets()
    , air_migration_offsets()
    , regional_migration_offsets()
    , sea_migration_offsets()
    , use_default_migration(false)
    {
    }

    bool
    MigrationInfoFactory::Configure(
        const Configuration* config
    )
    {
        initConfigTypeMap( "Enable_Air_Migration", &airplane_migration, Enable_Air_Migration_DESC_TEXT, false );
        initConfigTypeMap( "Enable_Regional_Migration", &regional_migration, Enable_Regional_Migration_DESC_TEXT, true );
        initConfigTypeMap( "Enable_Sea_Migration", &sea_migration, Enable_Sea_Migration_DESC_TEXT, true );
        initConfigTypeMap( "Enable_Local_Migration", &local_migration, Enable_Local_Migration_DESC_TEXT, true );
        initConfigTypeMap( "Air_Migration_Filename", &airmig_filename, Air_Migration_Filename_DESC_TEXT ); // migration only
        initConfigTypeMap( "Local_Migration_Filename", &localmig_filename, Local_Migration_Filename_DESC_TEXT ); // migration only
        initConfigTypeMap( "Regional_Migration_Filename", &regionmig_filename, Regional_Migration_Filename_DESC_TEXT ); // migration only
        initConfigTypeMap( "Sea_Migration_Filename", &seamig_filename, Sea_Migration_Filename_DESC_TEXT ); // migration only
        return JsonConfigurable::Configure( config );
    }

    bool
    MigrationInfo::Configure(
        const Configuration* config
    )
    {
        initConfigTypeMap( "Enable_Migration_Heterogeneity",  &is_heterogeneity_enabled, Enable_Migration_Heterogeneity_DESC_TEXT, true  );
        initConfigTypeMap( "x_Local_Migration", &x_localmigration, x_Local_Migration_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        initConfigTypeMap( "x_Sea_Migration", &x_seamigration, x_Sea_Migration_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        initConfigTypeMap( "x_Air_Migration", &x_airmigration, x_Air_Migration_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        initConfigTypeMap( "x_Regional_Migration", &x_regionmigration, x_Regional_Migration_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        return JsonConfigurable::Configure( config );
    }

    void MigrationInfo::PickMigrationStep(IIndividualHumanContext *traveler, float migration_rate_modifier, suids::suid &destination, int /*MigrationType*/ &migration_type, float &time) const
    {
        int index = 0;

        time = (float)randgen->expdist(migration_rate_modifier * migration_totalrate);

        float desttemp = randgen->e();
        while (desttemp > migration_rate_cdf[index]) // TODO: std::lower_bound would be preferable, although this code doesn't take much time overall.
        {
            index++;
        }

        destination = adjacent_nodes[index];
        migration_type = migration_types[index];
    }

    bool MigrationInfo::Initialize(std::istream *local_migration_data,
                                            std::istream *air_migration_data,
                                            std::istream *regional_migration_data,
                                            std::istream *sea_migration_data,
                                            boost::bimap<uint32_t, suids::suid> *nodeid_suid_map)
    {
        Configure( EnvPtr->Config );

        uint32_t tempIDarray[MAX_AIR_MIGRATION_DESTINATIONS] = {0}; // TODO: this assumes air-migration has the most # of destinations... could potentially break in the future
        double tempIDrates[MAX_AIR_MIGRATION_DESTINATIONS] = {0};

#ifdef _DEBUG
        static bool dump_map = false;
        if (EnvPtr->MPI.Rank == 0 && dump_map) {
            for (auto& entry : nodeid_suid_map->left)
            {
                LOG_ERR_F("map[%d] = %d\n", entry.first, entry.second.data);
            }
            dump_map = false;
        }
#endif

        try
        {
            if (local_migration_data)
            {
                LOG_DEBUG( "Reading local migration data...\n" );
                local_migration_data->read((char*) tempIDarray, MAX_LOCAL_MIGRATION_DESTINATIONS * sizeof(uint32_t));
                int num = local_migration_data->gcount();

                LOG_DEBUG_F( "%d bytes read from istream into int array, first value = %lu.\n", num, tempIDarray[0] );
                local_migration_data->read((char*) tempIDrates, MAX_LOCAL_MIGRATION_DESTINATIONS * sizeof(double));
                //LOG_DEBUG_F( "nodeid_suid_map.size() = %d\n", nodeid_suid_map->size() );

                for (int i = 0; i < MAX_LOCAL_MIGRATION_DESTINATIONS; i++)
                {
                    if (tempIDarray[i] > 0)
                    {
                        LOG_DEBUG_F( "Looking for map index (left) in nodeid_suid_map with value = %d\n", tempIDarray[i]);
                        adjacent_nodes.push_back(nodeid_suid_map->left.at(tempIDarray[i]));
                        migration_rate_cdf.push_back(tempIDrates[i] * x_localmigration); // migration tuning knob
                        migration_types.push_back(LOCAL_MIGRATION);
                    }
                }
            }

            // TODO: most nodes aren't airports, so there's a lot of unused space in the air-migration files... make them sparse like seaport?
            if (air_migration_data)
            {
                LOG_DEBUG( "Reading air migration data...\n" );
                air_migration_data->read((char*) tempIDarray, MAX_AIR_MIGRATION_DESTINATIONS * sizeof(uint32_t));
                air_migration_data->read((char*) tempIDrates, MAX_AIR_MIGRATION_DESTINATIONS * sizeof(double));

                for (int i = 0; i < MAX_AIR_MIGRATION_DESTINATIONS; i++)
                {
                    if (tempIDarray[i] > 0)
                    {
                        adjacent_nodes.push_back(nodeid_suid_map->left.at(tempIDarray[i]));
                        migration_rate_cdf.push_back(tempIDrates[i] * x_airmigration); // migration tuning knob
                        migration_types.push_back(AIR_MIGRATION);
                    }
                }
            }

            if (regional_migration_data)
            {
                LOG_DEBUG( "Reading regional migration data...\n" );
                regional_migration_data->read((char*) tempIDarray, MAX_REGIONAL_MIGRATION_DESTINATIONS * sizeof(uint32_t));
                regional_migration_data->read((char*) tempIDrates, MAX_REGIONAL_MIGRATION_DESTINATIONS * sizeof(double));

                for (int i = 0; i < MAX_REGIONAL_MIGRATION_DESTINATIONS; i++)
                {
                    if (tempIDarray[i] > 0)
                    {
                        adjacent_nodes.push_back(nodeid_suid_map->left.at(tempIDarray[i]));
                        migration_rate_cdf.push_back(tempIDrates[i] * x_regionmigration); // migration tuning knob
                        migration_types.push_back(REGIONAL_MIGRATION);
                    }
                }
            }

            if (sea_migration_data)
            {
                LOG_DEBUG( "Reading sea migration data...\n" );
                sea_migration_data->read((char*) tempIDarray, MAX_SEA_MIGRATION_DESTINATIONS * sizeof(uint32_t));
                sea_migration_data->read((char*) tempIDrates, MAX_SEA_MIGRATION_DESTINATIONS * sizeof(double));

                for (int i = 0; i < MAX_SEA_MIGRATION_DESTINATIONS; i++)
                {
                    if (tempIDarray[i] > 0)
                    {
                        adjacent_nodes.push_back(nodeid_suid_map->left.at(tempIDarray[i]));
                        migration_rate_cdf.push_back(tempIDrates[i] * x_seamigration); // migration tuning knob
                        migration_types.push_back(SEA_MIGRATION);
                    }
                }
            }
        }
        catch(std::out_of_range e)
        {
            // Is this the right thing to do?  In the future, could there ever be extra links in a migration file to nodes not in the suid-map for this simulation?
            LOG_ERR_F("Couldn't find matching suid for one of Node's migration links: %s\n", e.what() );
            return false;
        }

        //  Calculate total migration rate
        migration_totalrate = 0;
        for(int i = 0; i < migration_rate_cdf.size(); i++)
            migration_totalrate += migration_rate_cdf[i];

        if (migration_totalrate == 0)
        {
            LOG_WARN("migration_totalrate for node came to 0. This used to be an error, based on assumption that no node should be migrationally isolated. But this is allowed for testing scenarios.\n");
        }

        //  Set probability of each location
        migration_rate_cdf[0] /= migration_totalrate;
        for(int i = 1; i < migration_rate_cdf.size(); i++)
            migration_rate_cdf[i] = (migration_rate_cdf[i] / migration_totalrate) + migration_rate_cdf[i-1];

        // Values in the migration_rate_cdf[] are compared against values generated by RANDOMBASE::e(), which 
        // is guaranteed to create values between 0.0f and 1.0f.  We need to explicitly set the last value in
        // migration_rate_cdf to 1.0f, otherwise floating-point rounding errors might result in running past the
        // end of valid values in the array when picking a migration destination
        migration_rate_cdf[migration_rate_cdf.size() - 1] = 1.0f;

        return true;
    }


    MigrationInfoFactory * MigrationInfoFactory::CreateMigrationInfoFactory(boost::bimap<uint32_t, suids::suid> *nodeid_suid_map, const ::Configuration *config, const string idreference)
    {
        MigrationInfoFactory* factory = _new_ MigrationInfoFactory(nodeid_suid_map);
        if(!factory->Initialize(config, idreference))
        {
            delete factory;
            factory = NULL;
        }

        return factory;
    }

    bool MigrationInfoFactory::Initialize(const ::Configuration* config, const string idreference)
    {
        Configure( EnvPtr->Config );

        local_migration_file = NULL;
        air_migration_file = NULL;
        regional_migration_file = NULL;
        sea_migration_file = NULL;

        try
        {
            bool demographics_initial = GET_CONFIGURABLE(SimulationConfig)->demographics_initial;

            if(demographics_initial)
            {
                if(local_migration)
                {
                    if( localmig_filename.empty() )
                    {
                        throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Enable_Local_Migration", "1", "Local_Migration_Filename", "<empty>" );
                    }

                    string local_migration_filepath = FileSystem::Concat( EnvPtr->InputPath, localmig_filename);

                    if(!ParseMetadataForFile(local_migration_filepath, idreference, local_migration_offsets))
                        return false;
                    if(!OpenMigrationFile(local_migration_filepath, local_migration_offsets.size() * MAX_LOCAL_MIGRATION_DESTINATIONS * (sizeof(uint32_t) + sizeof(double)), local_migration_file))
                        return false;
                }

                if(airplane_migration)
                {
                    if( airmig_filename.empty() )
                    {
                        throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Enable_Air_Migration", "1", "Air_Migration_Filename", "<empty>" );
                    }

                    string air_migration_filepath = FileSystem::Concat( EnvPtr->InputPath, airmig_filename );

                    if(!ParseMetadataForFile(air_migration_filepath, idreference, air_migration_offsets))
                        return false;
                    if(!OpenMigrationFile(air_migration_filepath, air_migration_offsets.size() * MAX_AIR_MIGRATION_DESTINATIONS * (sizeof(uint32_t) + sizeof(double)), air_migration_file))
                        return false;
                }

                if(regional_migration)
                {
                    if( regionmig_filename.empty() )
                    {
                        throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Enable_Regional_Migration", "1", "Regional_Migration_Filename", "<empty>" );
                    }

                    string regional_migration_filepath = FileSystem::Concat( EnvPtr->InputPath, regionmig_filename );

                    if(!ParseMetadataForFile(regional_migration_filepath, idreference, regional_migration_offsets))
                        return false;
                    if(!OpenMigrationFile(regional_migration_filepath, regional_migration_offsets.size() * MAX_REGIONAL_MIGRATION_DESTINATIONS * (sizeof(uint32_t) + sizeof(double)), regional_migration_file))
                        return false;
                }

                if(sea_migration)
                {
                    if( seamig_filename.empty() )
                    {
                        throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Enable_Sea_Migration", "1", "Sea_Migration_Filename", "<empty>" );
                    }

                    string sea_migration_filepath = FileSystem::Concat( EnvPtr->InputPath, seamig_filename );

                    if(!ParseMetadataForFile(sea_migration_filepath, idreference, sea_migration_offsets))
                        return false;
                    if(!OpenMigrationFile(sea_migration_filepath, sea_migration_offsets.size() * MAX_SEA_MIGRATION_DESTINATIONS * (sizeof(uint32_t) + sizeof(double)), sea_migration_file))
                        return false;
                }
            }
            else // use default local migration to surrounding tiles, in 10x10 torus
            {
                LOG_INFO_F("Using generated migration for %s\n", idreference.c_str());
                use_default_migration = true;
            }
        }
        catch (json::Exception &e)
        {
            LOG_ERR_F("Exception during migration initialization:\n%s\n", e.what());
            return false;
        }

        return true;
    }


    bool MigrationInfoFactory::ParseMetadataForFile(string data_filepath, std::string idreference, hash_map<uint32_t, uint32_t> &node_offsets)
    {
        string metadata_filepath = data_filepath + ".json";

        Configuration* config = Configuration::Load(metadata_filepath);

        if (config == NULL)
        {
            // std::cerr << "Failed parsing migration metadata file " << metadata_filepath << std::endl;
            throw FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, metadata_filepath.c_str() );
        }

        if(!boost::iequals((string)((*config)["Metadata"]["IdReference"].As<json::String>()), idreference))
        {
            // std::cerr << "IdReference used to generate migration file " << data_filepath << " doesn't match the IdReference used for the demographics" << std::endl;
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "idreference", idreference.c_str(), "(string)((*config)[\"Metadata\"][\"IdReference\"].As<json::String>())", ((string)((*config)["Metadata"]["IdReference"].As<json::String>())).c_str() );
        }

        int num_nodes = (int)((*config)["Metadata"]["NodeCount"].As<json::Number>());

        string offsets_str = (string)((*config)["NodeOffsets"].As<json::String>());
        if(offsets_str.length() / 16 != num_nodes)
        {
            //std::cerr << "Format error encountered loading climate metadata file: " << metadata_filepath << endl;
            //std::cerr << "Length of NodeOffsets isn't consistent with \"NodeCount\" attribute" << endl;
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "offsets_str.length() / 16", offsets_str.length() / 16, "num_nodes", num_nodes );
        }

        uint32_t nodeid = 0, offset = 0;

        for(int n = 0; n < num_nodes; n++)
        {
#ifdef WIN32
            sscanf_s(offsets_str.substr(n * 16, 8).c_str(), "%x", &nodeid);
            sscanf_s(offsets_str.substr((n * 16) + 8, 8).c_str(), "%x", &offset);
#else
            sscanf(offsets_str.substr(n * 16, 8).c_str(), "%x", &nodeid);
            sscanf(offsets_str.substr((n * 16) + 8, 8).c_str(), "%x", &offset);
#endif
            node_offsets[nodeid] = offset;
        }

        delete config;
        return true;
    }

    bool MigrationInfoFactory::OpenMigrationFile(string filepath, size_t expected_size, std::ifstream * &file)
    {
        if( !FileSystem::FileExists( filepath ) )
        {
            throw FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, filepath.c_str() );
        }

        ifstream *newfile = new ifstream(filepath, std::ios::binary);

        if (newfile->fail())
        {
            throw FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, filepath.c_str() );
        }

        newfile->seekg(0, ios::end);
        int filelen = newfile->tellg();

        if(filelen != expected_size)
        {
            LOG_ERR_F("Detected wrong size for migration data file: %s\n", filepath.c_str());
            throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filepath.c_str() );
        }

        newfile->seekg(0, ios::beg);
        file = newfile;

        return true;
    }

    MigrationInfo* MigrationInfoFactory::CreateMigrationInfo(INodeContext *parent_node)
    {
        MigrationInfo* new_migration_info = NULL;

        suids::suid node_suid = parent_node->GetSuid();

        if(nodeid_suid_map->right.count(node_suid) == 0)
        {
            // ERROR: Couldn't find matching NodeID for suid " << node_suid.data << endl;
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "nodeid_suid_map->right.count(node_suid)", 0, "node_suid", node_suid.data );
        }

        uint32_t nodeid = nodeid_suid_map->right.at(node_suid);

        if(!use_default_migration)
        {
            const NodeDemographics * demographics = parent_node->GetDemographics();
            bool isAirport   = (*demographics)["NodeAttributes"]["Airport"].AsUint64() != 0;
            bool isRegionHub = (*demographics)["NodeAttributes"]["Region" ].AsUint64() != 0;
            bool isSeaport   = (*demographics)["NodeAttributes"]["Seaport"].AsUint64() != 0;

            // it's possible that all 4 migration-types are empty for a given node, i.e. this node is a "fortress/island"
            // node; in that case, just return null so this node is not considered for migration
            if(local_migration_offsets.count(nodeid) != 0 ||
                air_migration_offsets.count(nodeid) != 0 ||
                regional_migration_offsets.count(nodeid) != 0 ||
                sea_migration_offsets.count(nodeid) != 0)
            {
                if (local_migration_file)
                    local_migration_file->seekg(local_migration_offsets[nodeid], std::ios::beg);

                if (air_migration_file && isAirport)
                    air_migration_file->seekg(air_migration_offsets[nodeid], std::ios::beg);

                if (regional_migration_file && isRegionHub)
                    regional_migration_file->seekg(regional_migration_offsets[nodeid], std::ios::beg);

                if (sea_migration_file && isSeaport)
                    sea_migration_file->seekg(sea_migration_offsets[nodeid], std::ios::beg);

                new_migration_info = _new_ MigrationInfo(parent_node);

                if(!new_migration_info->Initialize(local_migration_file, 
                                                    isAirport ? air_migration_file : NULL,
                                                    isRegionHub ? regional_migration_file : NULL,
                                                    isSeaport ? sea_migration_file : NULL,
                                                    nodeid_suid_map))
                {
                    std::ostringstream msg;
                    msg << "Error initializing migration info for NodeID " << nodeid << endl;
                    throw InitializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
            }
        }
        else
        {
            int torus_size = (int)NodeDemographicsFactory::default_geography_torus_size;

            // Create a buffer which mirrors what would be in a migration file for this node

            //char buff[MAX_LOCAL_MIGRATION_DESTINATIONS * (sizeof(uint32_t) + sizeof(double))];
            std::ostringstream buff_oss;
            uint32_t nodedata[ MAX_LOCAL_MIGRATION_DESTINATIONS ];
            double ratedata[ MAX_LOCAL_MIGRATION_DESTINATIONS ];

            int offsets[]    = {  -(torus_size+1), -torus_size, -(torus_size-1),
                                         -1,                           1,
                                   (torus_size-1),  torus_size,  (torus_size+1)};

            double basicrate = 1.0f / MAX_LOCAL_MIGRATION_DESTINATIONS / 10; // on average, a person should go to one of the 8 surrounding nodes every 10 days, per Philip

            // correct offsets if on any of the edges (of numbering grid scheme, not the torus)

            if (nodeid % torus_size == 1) // left edge
            {
                offsets[0] += torus_size;
                offsets[3] += torus_size;
                offsets[5] += torus_size;
            }
            else if (nodeid % torus_size == 0) //right edge
            {
                offsets[2] -= torus_size;
                offsets[4] -= torus_size;
                offsets[7] -= torus_size;
            }

            if (nodeid <= (uint32_t)torus_size) // top edge
            {
                offsets[0] += torus_size * torus_size;
                offsets[1] += torus_size * torus_size;
                offsets[2] += torus_size * torus_size;
            }
            else if (nodeid > (uint32_t)(torus_size * (torus_size - 1))) // bottom edge
            {
                offsets[5] -= torus_size * torus_size;
                offsets[6] -= torus_size * torus_size;
                offsets[7] -= torus_size * torus_size;
            }

            LOG_DEBUG_F( "MAX_LOCAL_MIGRATION_DESTINATIONS = %d\n", MAX_LOCAL_MIGRATION_DESTINATIONS );
            for (int i = 0; i < MAX_LOCAL_MIGRATION_DESTINATIONS; i++)
            {
                release_assert(nodeid + offsets[i] >= 1);
                release_assert(nodeid + offsets[i] <= (uint32_t)(torus_size * torus_size));

                nodedata[i] = nodeid + offsets[i];
                //LOG_DEBUG_F( "Setting nodedata/buffer index %d to %lu\n", i, nodeid+offsets[i] );
                ratedata[i] = basicrate;
            }
            buff_oss.write((char*)nodedata, sizeof( nodedata ) );
            buff_oss.write((char*)ratedata, sizeof( ratedata ) );

            istringstream ss(buff_oss.str());
            //LOG_DEBUG_F( "Sanity test: first int in ss = %d\n", (int*)(buff_oss.str().c_str())[0] );

            new_migration_info = _new_ MigrationInfo(parent_node);
            if(!new_migration_info->Initialize(&ss, NULL, NULL, NULL, nodeid_suid_map))
            {
                // ERROR: "Error initializing migration info for NodeID " << nodeid << endl;
                std::ostringstream msg;
                msg << "Error initializing migration info for NodeID " << nodeid << endl;
                throw InitializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }

        return new_migration_info;
    }

    MigrationInfoFactory::~MigrationInfoFactory()
    {
        if (local_migration_file)
            local_migration_file->close();
        if (air_migration_file)
            air_migration_file->close();
        if (regional_migration_file)
            regional_migration_file->close();
        if (sea_migration_file)
            sea_migration_file->close();
    }


    MigrationInfo::~MigrationInfo() { }

    void
    MigrationInfo::SetContextTo(INodeContext* _parent) { parent = _parent; }

    const std::vector<suids::suid>&
    MigrationInfo::GetAdjacentNodes()
    const
    {
        return adjacent_nodes;
    }

    const std::vector<int /*MigrationType*/>& 
    MigrationInfo::GetMigrationTypes() const { return migration_types; }

    MigrationInfo::MigrationInfo( INodeContext * _parent ) 
        : parent(_parent) 
        , adjacent_nodes()
        , migration_rate_cdf()
        , migration_types()
        , migration_totalrate(0.0)
        , x_airmigration(1.0)
        , x_regionmigration(1.0)
        , x_seamigration(1.0)
        , x_localmigration(1.0)
        , is_heterogeneity_enabled(true)
    {
    }

    MigrationInfoFactory::MigrationInfoFactory(boost::bimap<uint32_t, suids::suid> *nodeid_suid_map) : use_default_migration(false)
    {
        this->nodeid_suid_map = nodeid_suid_map;
    }

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    // IJsonSerializable Interfaces
    void MigrationInfo::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();

        root->Insert("adjacent_nodes");
        root->BeginArray();
        for (auto& node_id : adjacent_nodes)
        {
            node_id.JSerialize(root, helper);
        }
        root->EndArray();

        root->Insert("migration_rate_cdf");
        helper->JSerialize(migration_rate_cdf,root);

        root->Insert("migration_types");
        helper->JSerialize(migration_types,root);

        root->Insert("migration_totalrate",migration_totalrate);

        root->EndObject();
    }
    void MigrationInfo::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
    }
#endif
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::MigrationInfo)
namespace Kernel
{
    template<class Archive>
    void serialize(Archive & ar, MigrationInfo& info, const unsigned int file_version)
    {
        ar & info.adjacent_nodes
           & info.migration_rate_cdf
           & info.migration_types;

        ar & info.migration_totalrate;
    }
}
#endif
