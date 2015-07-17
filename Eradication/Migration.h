/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <math.h>
#include <vector>
#include <string>
#include <map>

#include "BoostLibWrapper.h"

#include "RANDOM.h"
#include "Contexts.h"
#include "Sugar.h"
#include "CajunIncludes.h"
#include "Configure.h"

#ifdef __GNUC__
#include <ext/hash_map>
namespace std
{
     using namespace __gnu_cxx;
}
#else
#include <hash_map>
#endif

class Configuration;

namespace Kernel
{
    // TODO: tried to change various <int> template types to <MigrationType> and got a bunch of compile errors, at least
    //       some related to serialization/Persist stuff... go back and fix that later
    enum MigrationType 
    {
        LOCAL_MIGRATION     = 1,
        AIR_MIGRATION       = 2,
        REGIONAL_MIGRATION  = 3,
        SEA_MIGRATION       = 4
    };

    class MigrationInfo : public JsonConfigurable
    {
        // for JsonConfigurable stuff...
        GET_SCHEMA_STATIC_WRAPPER(MigrationInfo)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

    public:
        void PickMigrationStep(IIndividualHumanContext * traveler, float migration_rate_modifier, suids::suid &destination, int /*MigrationType*/ &migration_type, float &time) const;

        virtual ~MigrationInfo();
        void SetContextTo(INodeContext* _parent);

        const std::vector<suids::suid>& GetAdjacentNodes() const;
        const std::vector<int /*MigrationType*/>& GetMigrationTypes() const;
        bool IsHeterogeneityEnabled() const { return is_heterogeneity_enabled; }

    protected:
        INodeContext * parent;

        // would it make sense to split these up by migration-type?
        std::vector<suids::suid> adjacent_nodes;
        std::vector<float> migration_rate_cdf;
        std::vector<int /*MigrationType*/> migration_types;

        float migration_totalrate;

        float x_airmigration;
        float x_regionmigration;
        float x_seamigration;
        float x_localmigration;
        bool is_heterogeneity_enabled;

        friend class MigrationInfoFactory;

        MigrationInfo( INodeContext * _parent = nullptr );

        bool Initialize(std::istream *local_migration_data,
                        std::istream *air_migration_data,
                        std::istream *regional_migration_data,
                        std::istream *sea_migration_data,
                        boost::bimap<uint32_t, suids::suid> *nodeid_suid_map);

    private:

        bool Configure( const Configuration * config );

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
        friend class ::boost::serialization::access;

        template<class Archive>
        friend void serialize(Archive & ar, MigrationInfo& info, const unsigned int file_version);
        FORCE_POLYMORPHIC()
#endif
    };

    class MigrationInfoFactory : public JsonConfigurable
    {
    public:
        // for JsonConfigurable stuff...
        GET_SCHEMA_STATIC_WRAPPER(MigrationInfoFactory)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

        std::string airmig_filename;
        std::string localmig_filename;
        std::string regionmig_filename;
        std::string seamig_filename;

        static MigrationInfoFactory* CreateMigrationInfoFactory(boost::bimap<uint32_t, suids::suid> *nodeid_suid_map, const ::Configuration *config, const std::string idreference);
        ~MigrationInfoFactory();

        MigrationInfo* CreateMigrationInfo(INodeContext *parent_node);

        bool Configure( const Configuration * config );

    private:
        boost::bimap<uint32_t, suids::suid> *nodeid_suid_map;

        bool airplane_migration;
        bool regional_migration;
        bool sea_migration;
        bool local_migration;

        // data file stream handles for migration files
        std::ifstream *local_migration_file;
        std::ifstream *air_migration_file;
        std::ifstream *regional_migration_file;
        std::ifstream *sea_migration_file;

        // node offsets in migration files
        std::hash_map<uint32_t, uint32_t> local_migration_offsets;
        std::hash_map<uint32_t, uint32_t> air_migration_offsets;
        std::hash_map<uint32_t, uint32_t> regional_migration_offsets;
        std::hash_map<uint32_t, uint32_t> sea_migration_offsets;

        // TODO: might be nice to do something here to store offsets in a "map" sorted by value and then just pass a string/data-stream
        //      of the migration-data to the MigrationInfo that's being created, but none of the STL types seem to fulfill the requirements
        //      of being able to sort by value but search by key.

        //template<class _Ty>
        //struct offsets_less : public binary_function<_Ty, _Ty, bool>
        //{
        //    bool operator()(const _Ty& _Left, const _Ty& _Right) const
        //    {
        //        return (_Left < _Right);
        //    }
        //};

        //std::set<std::pair<uint32_t, uint32_t>, offsets_less<std::pair<uint32_t, uint32_t> > > localmigoffsets;

        bool use_default_migration;

        MigrationInfoFactory(boost::bimap<uint32_t, suids::suid> *nodeid_suid_map);
        MigrationInfoFactory();
        bool Initialize(const ::Configuration *config, const std::string idreference);
        bool ParseMetadataForFile(std::string data_filepath, std::string idreference, std::hash_map<uint32_t, uint32_t> &node_offsets);
        bool OpenMigrationFile(std::string filepath, size_t expected_size, std::ifstream * &file);
    };
}
