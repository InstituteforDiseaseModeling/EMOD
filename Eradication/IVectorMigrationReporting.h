
#pragma once

//#include "ISupports.h"
#include "suids.hpp"

namespace Kernel
{
    struct IVectorCohort;
    struct ISimulationContext;

    // Allows reports to get data on vector migration
    struct IVectorMigrationReporting : ISupports
    {
        virtual ~IVectorMigrationReporting() {};

        virtual void LogVectorMigration( ISimulationContext* pSim, 
                                         float currentTime, 
                                         const suids::suid& nodeSuid, 
                                         IVectorCohort* pivc ) = 0;
    };
}
