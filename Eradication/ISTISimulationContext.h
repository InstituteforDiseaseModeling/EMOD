
#pragma once

#include "suids.hpp"
#include "ISupports.h"

namespace Kernel
{
    struct IRelationship;

    struct ISTISimulationContext : virtual ISupports
    {
        virtual void AddTerminatedRelationship( const suids::suid& nodeSuid, const suids::suid& relId ) = 0;
        virtual bool WasRelationshipTerminatedLastTimestep( const suids::suid& relId ) const = 0;
        virtual suids::suid GetNextCoitalActSuid() = 0;
        virtual void AddEmigratingRelationship( IRelationship* pRel ) = 0;
    };
}
