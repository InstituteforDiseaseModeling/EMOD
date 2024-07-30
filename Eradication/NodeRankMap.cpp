
#include "stdafx.h"
#include <fstream>
#include <memory>

#include "NodeRankMap.h"
#include "Log.h"
#include "FileSystem.h"
#include "INodeInfo.h"
#include "JsonRawWriter.h"
#include "JsonRawReader.h"
#include "LoadBalanceScheme.h"
#include "MpiDataExchanger.h"
#include "BinaryArchiveWriter.h"
#include "BinaryArchiveReader.h"
#include "Debug.h"
#include "IdmMpi.h"
#include "INodeContext.h"

SETUP_LOGGING( "NodeRankMap" )

namespace Kernel 
{
    NodeRankMap::NodeRankMap()
        : initialLoadBalanceScheme(nullptr)
        , rankMap()
        , pNodeInfoFactory(nullptr)
        , nodes_in_my_rank()
        , m_Buffer(nullptr)
        , m_BufferSize(0)
        , nodesBySuid()
    { 
    }

    NodeRankMap::~NodeRankMap()
    {
        delete initialLoadBalanceScheme ;
        initialLoadBalanceScheme = nullptr ;

        for( auto entry : rankMap )
        {
            delete entry.second;
        }
        rankMap.clear();

        // don't own pNodeInfoFactory
        pNodeInfoFactory = nullptr ;

        free( m_Buffer );
    }

/* TODO
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
*/

    bool NodeRankMap::MergeMaps()
    {
        RankMap_t mergedMap;
        // Style note: exceptions used locally here to obey MPI semantics but also allow me to detect failure without an overly complex return-code mechanism
        try
        {
            LOG_DEBUG_F("%s\n", __FUNCTION__);
            mergedMap = rankMap;

            if (EnvPtr->MPI.NumTasks > 1)
            {
                auto json_writer = new JsonRawWriter();
                IArchive& writer = *static_cast<IArchive*>(json_writer);
                size_t count = rankMap.size();
                writer.startArray( count );
                LOG_VALID_F( "Serializing %d suid-rank map entries.\n", count );
                for (auto& entry : rankMap)
                {
                    INodeInfo* pni = entry.second;
                    writer.startObject();
                    // ---------------------------------------------------------------
                    // --- true => write all of the data about the node since this is
                    // --- the first time and the other nodes will need it
                    // ---------------------------------------------------------------
                    writer.labelElement("value"); pni->serialize(writer, true);
                    writer.endObject();
                }
                writer.endArray();

                for (int rank = 0; rank < EnvPtr->MPI.NumTasks; ++rank)
                {
                    if (rank == EnvPtr->MPI.Rank)
                    {
                        const char* buffer = writer.GetBuffer();
                        size_t size = writer.GetBufferSize();
                        LOG_VALID_F( "Broadcasting serialized map (%d bytes)\n", size );
                        EnvPtr->MPI.p_idm_mpi->PostChars( const_cast<char*>(buffer), size, rank );
                    }
                    else
                    {
                        std::vector<char> received;
                        EnvPtr->MPI.p_idm_mpi->GetChars( received, rank );
                        auto json_reader = new JsonRawReader( received.data() );
                        IArchive& reader = *static_cast<IArchive*>(json_reader);
                        reader.startArray( count );
                            LOG_VALID_F( "Merging %d suid-rank map entries from rank %d\n", count, rank );
                            for (size_t i = 0; i < count; ++i)
                            {
                                INodeInfo* pni = this->pNodeInfoFactory->CreateNodeInfo();
                                reader.startObject();
                                    // ---------------------------------------------------------------
                                    // --- true => read all of the data about the node since this is
                                    // --- the first time this core needs the data about the nodes
                                    // ---------------------------------------------------------------
                                    reader.labelElement( "value" ); pni->serialize( reader, true );
                                reader.endObject();
                                mergedMap[ pni->GetSuid() ] = pni; // own memory
                            }
                        reader.endArray();
                        delete json_reader;
                    }
                }

                delete json_writer;
            }
        }
        catch (std::exception &e)
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, e.what() );
        }

        rankMap = mergedMap;

        // ---------------------------------------------------------------------
        // --- Save a list of INodeInfo objects that are hosted on this core.
        // --- We need to send this list to the other cores each time step.
        // ---------------------------------------------------------------------
        for( auto& entry : rankMap )
        {
            if ( entry.second->GetRank() == EnvPtr->MPI.Rank )
            {
                nodes_in_my_rank.push_back( entry.second );
            }

            const suids::suid& suid = entry.first;
            INodeInfo* ini = entry.second;
            int32_t rank = ini->GetRank();
            if (suid.data >= nodesBySuid.size()) nodesBySuid.resize(suid.data + 1);
            nodesBySuid.data()[suid.data] = ini;
            if (suid.data >= ranksBySuid.size()) ranksBySuid.resize(suid.data + 1);
            ranksBySuid.data()[suid.data] = rank;
        }
        return true;
    }

    void NodeRankMap::Sync( IdmDateTime& currentTime )
    {
        // ------------------------------------------------------------------------
        // --- The data to be sent is the same for all processes, hence, we want to
        // --- serialize it once and then send that same chunk to each process.
        // ------------------------------------------------------------------------
        auto binary_writer = new BinaryArchiveWriter();
        IArchive* writer = static_cast<IArchive*>(binary_writer);
        size_t count = nodes_in_my_rank.size();
        writer->startArray( count );
        for( size_t i = 0; i < count; ++i )
        {
            int id = nodes_in_my_rank[i]->GetSuid().data;
            INodeInfo* pni = nodes_in_my_rank[i] ;
            writer->startObject();
                // ---------------------------------------------------------
                // --- false => Since we are just updating the other cores, 
                // --- we only need to send the data that has changed.
                // ----------------------------------------------------------
                writer->labelElement( "key"   ) & id; // send id so receive can use to put in map
                writer->labelElement( "value" ); pni->serialize( *writer, false );
            writer->endObject();
        }
        writer->endArray();

        char* buffer_temp = const_cast<char*>(writer->GetBuffer());
        unsigned char* buffer = (unsigned char*)buffer_temp;
        size_t buffer_size = writer->GetBufferSize();

        // -----------------------------------------------------
        // --- Define functions to be called by MpiDataExchanger
        // -----------------------------------------------------
        WithSelfFunc to_self_func = [this](int myRank) 
        { 
            //do nothing
        }; 

        SendToOthersFunc to_others_func = [this,buffer,buffer_size](IArchive* writer, int toRank)
        {
            writer->labelElement("ArchivedNodeInfoData"); writer->serialize( buffer, buffer_size );
        };

        ClearDataFunc clear_data_func = [this](int rank)
        {
            //do nothing
        };

        ReceiveFromOthersFunc from_others_func = [this](IArchive* reader, int fromRank)
        {
            // -------------------------------------
            // --- Read the buffer out of the reader
            // -------------------------------------
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! I'd really like to get access to this buffer directly
            // !!! so we don't have to allocate the memory
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            size_t data_size = 0;
            reader->startArray( data_size );

            if( (m_Buffer == nullptr) || (data_size > m_BufferSize) )
            {
                delete m_Buffer;

                m_BufferSize = data_size;
                m_Buffer = (unsigned char*)malloc( m_BufferSize );
                if( m_Buffer == nullptr )
                {
                    throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_Buffer", "unsigned char" );
                }
            }
            memset( m_Buffer, 0, m_BufferSize );

            for (size_t i = 0; i < data_size; ++i)
            {
                (*reader) & m_Buffer[i];
            }

            reader->endArray();

            // -----------------------------------------
            // --- Deserialize the data from the buffer
            // -----------------------------------------
            BinaryArchiveReader binary_reader_converter( (char*)m_Buffer, data_size );
            IArchive* converter = static_cast<IArchive*>(&binary_reader_converter);
            size_t count=0;
            converter->startArray( count );
            for (size_t i = 0; i < count; ++i)
            {
                suids::suid id;
                converter->startObject();
                    // ---------------------------------------------------------
                    // --- false => Since we are just updating the INodeInfo objects 
                    // --- for the nodes on the other cores, we only need to read 
                    // --- the data that has changed.
                    // ----------------------------------------------------------
                    converter->labelElement("key") & id;
                    INodeInfo* pni = rankMap.at( id );
                    converter->labelElement( "value" ); pni->serialize( *converter, false );
                converter->endObject();
            }
            converter->endArray();
        };

        MpiDataExchanger exchanger( "NodeRankMap", to_self_func, to_others_func, from_others_func, clear_data_func );
        exchanger.ExchangeData( currentTime );

        delete binary_writer;
    }

    void NodeRankMap::SetInitialLoadBalanceScheme( IInitialLoadBalanceScheme *ilbs ) 
    {
        initialLoadBalanceScheme = ilbs; 
    }

    void NodeRankMap::SetNodeInfoFactory( INodeInfoFactory* pnif )
    {
        pNodeInfoFactory = pnif ;
    }

    int NodeRankMap::GetRankFromNodeSuid(suids::suid node_id)
    {
        int rank = ranksBySuid.data()[node_id.data];
        return rank;
    }

    size_t NodeRankMap::Size()
    {
        return rankMap.size();
    }

    void NodeRankMap::Add( int rank, INodeContext* pNC )
    {
        INodeInfo* pni = pNodeInfoFactory->CreateNodeInfo( rank, pNC );
        suids::suid suid = pNC->GetSuid();
        rankMap.insert(RankMapEntry_t(suid, pni));
        if (suid.data >= nodesBySuid.size()) nodesBySuid.resize(suid.data + 1);
        nodesBySuid.data()[suid.data] = pni;
        if (suid.data >= ranksBySuid.size()) ranksBySuid.resize(suid.data + 1);
        ranksBySuid.data()[suid.data] = rank;
    }

    void NodeRankMap::Update( INodeContext* pNC )
    {
        rankMap.at( pNC->GetSuid() )->Update( pNC );
    }

    INodeInfo& NodeRankMap::GetNodeInfo( const suids::suid& node_suid )
    {
        INodeInfo* ptr = nodesBySuid.data()[node_suid.data];
        return *ptr;
    }

    const NodeRankMap::RankMap_t&
    NodeRankMap::GetRankMap() const 
    { 
        return rankMap;
    }

    int NodeRankMap::GetInitialRankFromNodeId( ExternalNodeId_t node_id )
    {
        if( initialLoadBalanceScheme ) 
        { 
            return initialLoadBalanceScheme->GetInitialRankFromNodeId(node_id); 
        }
        else 
        { 
            return node_id % EnvPtr->MPI.NumTasks; 
        }
    }

    suids::suid NodeRankMap::GetSuidFromExternalID( uint32_t externalNodeId ) const
    {
        for( auto entry : rankMap )
        {
            if ( entry.second->GetExternalID() == externalNodeId )
            {
                return entry.second->GetSuid();
            }
        }
        std::ostringstream msg;
        msg << "Could not find externalNodeId = " << externalNodeId ;
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
    }


    const char*
    NodeRankMap::merge_duplicate_key_exception::what()
    const throw()
    { 
        return "Duplicate key in map merge\n"; 
    }

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
            if (!(mergedMap.insert(entry).second))
                throw(merge_duplicate_key_exception());
        }

        for (auto& entry : y)
        {
            if (!(mergedMap.insert(entry).second)) // .second is false if the key already existed
                throw(merge_duplicate_key_exception());
        }

        return mergedMap;
    }
}
