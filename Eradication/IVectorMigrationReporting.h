/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

//#include "ISupports.h"
#include "suids.hpp"

namespace Kernel
{
    struct IVectorCohort;
    struct ISimulationContext ;

    // Allows reports to get data on vector migration
    struct IVectorMigrationReporting// : ISupports
    {
        virtual ~IVectorMigrationReporting() {};

        virtual void LogVectorMigration( ISimulationContext* pSim, 
                                         float currentTime, 
                                         const suids::suid& nodeSuid, 
                                         IVectorCohort* pivc ) = 0;
    };
}
