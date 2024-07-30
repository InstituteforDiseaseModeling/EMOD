
#pragma once

#include "stdafx.h"
#include "IMigrationInfo.h"

namespace Kernel
{
    struct IVectorSimulationContext;

    // Extend the IMigrationInfo interface to support migrating vectors
    struct IDMAPI IMigrationInfoVector : virtual IMigrationInfo
    {
        // The rates for vectors change change due to the 
        // change in human population and vector habitat
        virtual void UpdateRates( const suids::suid& rThisNodeId, 
                                  const std::string& rSpeciesID, 
                                  IVectorSimulationContext* pivsc ) = 0;
    };

    struct IDMAPI IMigrationInfoFactoryVector : virtual IMigrationInfoFactory
    {
        virtual IMigrationInfoVector* CreateMigrationInfoVector(
            INodeContext *parent_node, 
            const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) = 0;

        virtual bool IsVectorMigrationEnabled() const = 0;
    };
}
