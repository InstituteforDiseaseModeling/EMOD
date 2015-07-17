/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "sys/stat.h"
#include <fstream>

#include "NodeRankMap.h"
#include "Log.h"
#include "FileSystem.h"

static const char * _module = "NodeRankMap";
namespace Kernel {

    bool LegacyFileInitialLoadBalanceScheme::Initialize( std::string loadbalancefilename, uint32_t expected_num_nodes )
    {
        bool initialized = false;

        std::ifstream loadbalancefile;
        if(loadbalancefilename.length()) 
        {
            if( FileSystem::FileExists( loadbalancefilename ) )
            {
                loadbalancefile.open(loadbalancefilename, std::ios::binary);
            }
        }

        if(loadbalancefile.is_open()) 
        {
            loadbalancefile.seekg(0, std::ios::end);
            int filelen = (int)loadbalancefile.tellg();
            int expected_size = sizeof(uint32_t) + (expected_num_nodes * (sizeof(uint32_t) + sizeof(float)));

            if(filelen == expected_size)
            {
                uint32_t num_nodes;
                loadbalancefile.seekg(0, std::ios::beg);
                loadbalancefile.read((char*)&num_nodes, 1*sizeof(num_nodes));

                if(num_nodes == expected_num_nodes)
                {
                    LOG_INFO_F("Opened %s, reading balancing data for %d nodes...\n", loadbalancefilename.c_str(), num_nodes);

                    std::vector<uint32_t> nodeids(num_nodes);
                    std::vector<float> balance_scale(num_nodes);

                    loadbalancefile.read((char*)&nodeids[0], (num_nodes)*sizeof(uint32_t));
                    loadbalancefile.read((char*)&balance_scale[0], (num_nodes)*sizeof(float));

                    for(uint32_t i = 0; i < num_nodes; i++)
                    {
                        initialNodeRankMapping[nodeids[i]] = (int)(EnvPtr->MPI.NumTasks*balance_scale[i]);
                    }
                    LOG_INFO("Static initial load balancing scheme initialized.\n");

                    initialized = true;
                }
                else
                {
                    LOG_WARN_F( "Malformed load-balancing file: %s.  Node-count specified in file (%d) doesn't match expected number of nodes (%d)\n", loadbalancefilename.c_str(), num_nodes, expected_num_nodes );
                }
            }
            else
            {
                LOG_WARN_F( "Problem with load-balancing file: %s.  File-size (%d) doesn't match expected size (%d) given number of nodes in demographics file\n", loadbalancefilename.c_str(), filelen, expected_size );
            }

            loadbalancefile.close();
        }
        else
        {
            LOG_WARN_F( "Failed to open load-balancing file: %s\n", loadbalancefilename.c_str() );
        }

        return initialized;
    }


    std::string NodeRankMap::ToString()
    {
        std::stringstream strstr;
        strstr << "{ NodeRankMap:" << std::endl;
        for (auto& entry : rankMap)
        {
            strstr << "[" << entry.first.data << "," << entry.second << "]" << std::endl;
        }
        strstr << "}" << std::endl;
        return strstr.str();
    }

    bool NodeRankMap::MergeMaps()
    {

        RankMap_t mergedMap;
        // Style note: exceptions used locally here to obey MPI semantics but also allow me to detect failure without an overly complex return-code mechanism
        try
        {
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
            mergedMap = all_reduce(*(EnvPtr->MPI.World), rankMap, map_merge());
#else
            LOG_DEBUG_F("%s\n", __FUNCTION__);
            // USE_BOOST_SERIALIZATION: do something on the single machine without involving serialization
            mergedMap = rankMap;

            // Serialize myself
            IJsonObjectAdapter* writer = CreateJsonObjAdapter();
            writer->CreateNewWriter();
            JSerializer* helper = new JSerializer();
            writer->BeginArray();
            for (auto& entry : rankMap)
            {
                writer->BeginArray();
                (entry.first).JSerialize(writer, helper);
                writer->Add(uint32_t(entry.second));
                writer->EndArray();
            }
            writer->EndArray();

            // Send to all _other_ tasks
            const char* text = writer->ToString();
            LOG_DEBUG_F("%s: '%s'\n", __FUNCTION__, text);
            size_t length = strlen(text) + 1;
            MPI_Request request;
            for (int destination = 0; destination < EnvPtr->MPI.NumTasks; destination++)
            {
                if (destination != EnvPtr->MPI.Rank)
                {
                    LOG_DEBUG_F("%s: Sending node<->rank map to %02d\n", __FUNCTION__, destination);
                    MPI_Isend((void*)text, int(length), MPI_CHAR, destination, 0, MPI_COMM_WORLD, &request);
                }
            }

            // Receive node<->rank mapping from all _other_ tasks
            IJsonObjectAdapter* json = CreateJsonObjAdapter();
            vector<char> buffer;
            for (int count = 0; count < (EnvPtr->MPI.NumTasks - 1); count++)
            {
                MPI_Status status;
                MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                int size;
                MPI_Get_count(&status, MPI_CHAR, &size);
                if (buffer.size() < size)
                {
                    buffer.resize(size);
                }
                int source = status.MPI_SOURCE;
                LOG_DEBUG_F("%s: Processing message %02d from rank %02d.\n", __FUNCTION__, count, source);
                MPI_Request request;
                MPI_Irecv(buffer.data(), size, MPI_CHAR, source, 0, MPI_COMM_WORLD, &request);
                LOG_DEBUG_F("%s: '%s'\n", __FUNCTION__, buffer.data());

                json->Parse(buffer.data());
                for (IndexType i = 0; i < IndexType(json->GetSize()); i++)
                {
                    IJsonObjectAdapter* element = (*json)[i];
                    IJsonObjectAdapter* suid_entry = (*element)[IndexType(0)];
                    suids::suid node_suid;
                    node_suid.JDeserialize(suid_entry, helper);
                    IJsonObjectAdapter* rank_entry = (*element)[IndexType(1)];
                    int32_t rank = int32_t(*rank_entry);
                    mergedMap[node_suid] = rank;
                    delete rank_entry;
                    delete suid_entry;
                    delete element;
                }
                LOG_DEBUG_F("%s: Finished processing message %d.\n", __FUNCTION__, count);
            }
            LOG_DEBUG_F("%s: Finished node rank map merge.\n", __FUNCTION__);
            delete json;
            delete helper;
            delete writer;
            LOG_DEBUG_F("%s: Completed cleanup.\n", __FUNCTION__);
#endif
        }
        catch (std::exception &e)
        {
            LOG_ERR_F("MergeMaps() exception: %s\n",e.what());
            return false; // return failure; program must not continue if merge was invalid
        }

        rankMap = mergedMap;

        return true;
    }

    CheckerboardInitialLoadBalanceScheme::CheckerboardInitialLoadBalanceScheme()
    : num_ranked(0) { }

    int
    CheckerboardInitialLoadBalanceScheme::GetInitialRankFromNodeId(node_id_t node_id)
    { return (num_ranked++) % EnvPtr->MPI.NumTasks; }

    StripedInitialLoadBalanceScheme::StripedInitialLoadBalanceScheme()
    : num_ranked(0), num_nodes(0)
    {}

    void StripedInitialLoadBalanceScheme::Initialize(uint32_t in_num_nodes) { num_nodes = in_num_nodes; }

    int
    StripedInitialLoadBalanceScheme::GetInitialRankFromNodeId(node_id_t node_id)
    { return (int)(((float)(num_ranked++) / num_nodes) * EnvPtr->MPI.NumTasks); }

    int LegacyFileInitialLoadBalanceScheme::GetInitialRankFromNodeId(node_id_t node_id)
    { return initialNodeRankMapping[node_id]; }

    NodeRankMap::NodeRankMap() : initialLoadBalanceScheme(NULL) { }

    void NodeRankMap::SetInitialLoadBalanceScheme(IInitialLoadBalanceScheme *ilbs) { initialLoadBalanceScheme = ilbs; } 

    int NodeRankMap::GetRankFromNodeSuid(suids::suid node_id) { return rankMap[node_id]; } 

    size_t NodeRankMap::Size() { return rankMap.size(); }

    void NodeRankMap::Add(suids::suid node_suid, int rank) { rankMap.insert(RankMapEntry_t(node_suid, rank)); }

    const NodeRankMap::RankMap_t&
    NodeRankMap::GetRankMap() const { return rankMap; }

    int NodeRankMap::GetInitialRankFromNodeId(node_id_t node_id)
    {
        if (initialLoadBalanceScheme) { return initialLoadBalanceScheme->GetInitialRankFromNodeId(node_id); }
        else { return node_id % EnvPtr->MPI.NumTasks; }
    }

    const char*
    NodeRankMap::merge_duplicate_key_exception::what()
    const throw()
    { return "Duplicate key in map merge\n"; }

    NodeRankMap::RankMap_t
    NodeRankMap::map_merge::operator()(
        const RankMap_t& x,
        const RankMap_t& y
    )
    const
    {
        RankMap_t mergedMap;

        for (auto& entry : x)
        {
            if(!(mergedMap.insert(entry).second))
                throw(merge_duplicate_key_exception());
        }

        for (auto& entry : y)
        {
            if(!(mergedMap.insert(entry).second)) // .second is false if the key already existed
                throw(merge_duplicate_key_exception());
        }

        return mergedMap;
    }

#if USE_JSON_SERIALIZATION || USE_JSON_MPI

    // IJsonSerializable Interfaces
    void NodeRankMap::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();
        root->Insert("rankMap");
        root->BeginArray();
        for (auto& entry : rankMap)
        {
            root->BeginArray();
            ((suids::suid)(entry.first)).JSerialize(root, helper);
            root->Add(entry.second);
            root->EndArray();
        }
        root->EndArray();
        root->EndObject();
    }

    void NodeRankMap::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
        IJsonObjectAdapter* map = root->GetArray("rankMap");
        IJsonObjectAdapter* element;
        IJsonObjectAdapter* id;
        suids::suid node_id;
        for (unsigned int i = 0; i < map->GetSize(); i++)
        {
            element = (*map)[i];
            id = (*element)[(IndexType)0];
            node_id.JDeserialize(id, helper);
            int rank = ((*element)[1])->AsInt();
            rankMap[node_id] = rank;
            delete id;
            delete element;
        }
    }

#endif
}

#if USE_BOOST_SERIALIZATION
namespace Kernel
{
    template<typename Archive>
    void serialize(Archive & ar, NodeRankMap& nrm, const unsigned int file_version)
    {
        ar & nrm.rankMap;
    }
}
#endif
