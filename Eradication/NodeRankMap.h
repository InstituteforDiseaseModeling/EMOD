/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include <string>
#include <list>
#include <map>
#include <functional>
#include "BoostLibWrapper.h"
#include "Environment.h"
#include "suids.hpp"

namespace Kernel
{
    typedef uint32_t node_id_t;

    struct IInitialLoadBalanceScheme
    {
        virtual int GetInitialRankFromNodeId(node_id_t node_id) = 0;
    };

    class CheckerboardInitialLoadBalanceScheme : public IInitialLoadBalanceScheme
    {
    public:
        CheckerboardInitialLoadBalanceScheme();

        virtual int GetInitialRankFromNodeId(node_id_t node_id);

    protected:
        uint32_t num_ranked;
    };

    class StripedInitialLoadBalanceScheme : public IInitialLoadBalanceScheme
    {
    public:
        StripedInitialLoadBalanceScheme();

        void Initialize(uint32_t in_num_nodes);

        virtual int GetInitialRankFromNodeId(node_id_t node_id);

    protected:
        uint32_t num_nodes;
        uint32_t num_ranked;
    };

    class LegacyFileInitialLoadBalanceScheme : public IInitialLoadBalanceScheme
    {
    public:
        bool Initialize(std::string loadbalancefilename, uint32_t expected_num_nodes);

        virtual int GetInitialRankFromNodeId(node_id_t node_id);

    protected:
        std::map<node_id_t, int> initialNodeRankMapping;
    };


    /*
    TODO: 
    x leave node rank with map impl for now, hash_map to be implemented later if needed for performance
    */

    // base class for node->rank maps
    class NodeRankMap 
    {
    public:
        NodeRankMap();

        // NOTE: the initial scheme object is NOT serialized or cleaned up by this class. it is ONLY for initializing the node rank map the first time it is populated
        void SetInitialLoadBalanceScheme(IInitialLoadBalanceScheme *ilbs);

        int GetRankFromNodeSuid(suids::suid node_id);

        size_t Size();

        // node rank maps will be built up as nodes are added during populate()
        // then other ranks need to find out that our nodes do indeed live with us
        // therefore, we will send all the other nodes a copy of our map to be merged
        // merge maps on multiple processors
        bool MergeMaps();

        // this function encapsulates the initial mapping from node id on disk to rank.
        // although node ids should be simulation-unique we still track nodes by our own suid 
        // system. this may not ever be strictly advantageous but it might make it easier to, 
        // e.g., generate new nodes later in the simulation for example
        int GetInitialRankFromNodeId(node_id_t node_id);

        void Add(suids::suid node_suid, int rank);

        std::string ToString();

        // hack: to let us get the complete list of nodes
        typedef std::map<suids::suid, int> RankMap_t;
        typedef std::pair<suids::suid, int> RankMapEntry_t;

        const RankMap_t& GetRankMap() const;

    private:
        IInitialLoadBalanceScheme *initialLoadBalanceScheme;

        RankMap_t rankMap;

        struct merge_duplicate_key_exception : public std::exception
        {
            virtual const char* what() const throw();
        };

        struct map_merge : public std::binary_function<RankMap_t, RankMap_t, RankMap_t>
        {
            RankMap_t operator()(const RankMap_t& x, const RankMap_t& y) const;
        };

#if USE_JSON_SERIALIZATION
    public:
        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
    private:
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, NodeRankMap& nrm, const unsigned int file_version);
#endif
        ///////////////////////////////////////////////////////////////////////////
    };
}
