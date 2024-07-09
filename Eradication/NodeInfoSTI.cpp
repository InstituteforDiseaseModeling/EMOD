
#include "stdafx.h"
#include "NodeInfoSTI.h"
#include "IArchive.h"

namespace Kernel
{
    NodeInfoSTI::NodeInfoSTI()
        : NodeInfo()
        , m_TerminatedLastTimestepAdd()
        , m_TerminatedLastTimestepGet()
    {
    }

    NodeInfoSTI::NodeInfoSTI( int rank, INodeContext* pNC )
        : NodeInfo( rank, pNC )
        , m_TerminatedLastTimestepAdd()
        , m_TerminatedLastTimestepGet()
    {
    }

    NodeInfoSTI::~NodeInfoSTI()
    {
    }

    void NodeInfoSTI::Update( INodeContext* pNC )
    {
        NodeInfo::Update( pNC );
        m_TerminatedLastTimestepGet = m_TerminatedLastTimestepAdd;
        m_TerminatedLastTimestepAdd.clear();
    }

    void NodeInfoSTI::AddTerminatedRelationship( const suids::suid& individualId )
    {
        m_TerminatedLastTimestepAdd.insert( std::make_pair( individualId, individualId ) );
    }

    bool NodeInfoSTI::WasRelationshipTerminatedLastTimestep( const suids::suid& individualId ) const
    {
        return (m_TerminatedLastTimestepGet.count(individualId) > 0);
    }

    void NodeInfoSTI::serialize(IArchive& ar, bool firstTime )
    {
        NodeInfo::serialize( ar, firstTime );
        ar.labelElement("m_TerminatedLastTimestepGet") & m_TerminatedLastTimestepGet;
    }
}
