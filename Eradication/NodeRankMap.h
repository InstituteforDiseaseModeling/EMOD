
#pragma once
#include <map>
#include <vector>
#include "suids.hpp"
#include "ExternalNodeId.h"

namespace Kernel
{
    struct INodeContext;
    struct INodeInfo;
    struct INodeInfoFactory;
    struct IInitialLoadBalanceScheme;
    struct IdmDateTime;

    /*
    TODO: 
    x leave node rank with map impl for now, unordered_map to be implemented later if needed for performance
    */

    // base class for node->rank maps
    class NodeRankMap 
    {
    public:
        NodeRankMap();
        ~NodeRankMap();

        // NOTE: the initial scheme object is NOT serialized.  It is ONLY for initializing the node rank map the first time it is populated
        void SetInitialLoadBalanceScheme( IInitialLoadBalanceScheme *ilbs );

        void SetNodeInfoFactory( INodeInfoFactory* pnif );

        int GetRankFromNodeSuid(suids::suid node_id);

        suids::suid GetSuidFromExternalID( ExternalNodeId_t externalNodeId ) const ;

        size_t Size();

        // node rank maps will be built up as nodes are added during populate()
        // then other ranks need to find out that our nodes do indeed live with us
        // therefore, we will send all the other nodes a copy of our map to be merged
        // merge maps on multiple processors
        bool MergeMaps();

        void Sync( IdmDateTime& currentTime );

        // this function encapsulates the initial mapping from node id on disk to rank.
        // although node ids should be simulation-unique we still track nodes by our own suid 
        // system. this may not ever be strictly advantageous but it might make it easier to, 
        // e.g., generate new nodes later in the simulation for example
        int GetInitialRankFromNodeId( ExternalNodeId_t node_id );

        void Add( int rank, INodeContext* pNC );
        void Update( INodeContext* pNC );

        INodeInfo& GetNodeInfo( const suids::suid& node_suid );

//        std::string ToString();

        typedef std::map< suids::suid, INodeInfo*> RankMap_t;
        typedef std::pair<suids::suid, INodeInfo*> RankMapEntry_t;

        const RankMap_t& GetRankMap() const;

    private:
        IInitialLoadBalanceScheme *initialLoadBalanceScheme;

        RankMap_t rankMap;
        INodeInfoFactory* pNodeInfoFactory;
        std::vector<INodeInfo*> nodes_in_my_rank;
        unsigned char* m_Buffer;
        uint32_t m_BufferSize;

        std::vector<INodeInfo*> nodesBySuid;
        std::vector<int32_t> ranksBySuid;

        struct merge_duplicate_key_exception : public std::exception
        {
            virtual const char* what() const throw();
        };

        struct map_merge
        {
            RankMap_t operator()(const RankMap_t& x, const RankMap_t& y) const;
        };
    };
}
