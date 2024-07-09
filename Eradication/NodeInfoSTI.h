
#pragma once

#include <map>
#include "NodeInfo.h"

namespace Kernel
{
    class NodeInfoSTI : public NodeInfo
    {
    public:
        NodeInfoSTI();
        NodeInfoSTI( int rank, INodeContext* pNC );
        ~NodeInfoSTI();

        // NodeInfo Methods
        virtual void Update( INodeContext* pNC ) override;
        virtual void serialize( IArchive& ar, bool firstTime ) override;

        // STI methods
        virtual void AddTerminatedRelationship( const suids::suid& relId );
        virtual bool WasRelationshipTerminatedLastTimestep( const suids::suid& relId ) const;

    private:
        std::map<suids::suid,suids::suid> m_TerminatedLastTimestepAdd;
        std::map<suids::suid,suids::suid> m_TerminatedLastTimestepGet;
    };
}
