/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
