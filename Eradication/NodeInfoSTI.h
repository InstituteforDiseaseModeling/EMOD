/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
