/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ISupports.h"
#include "SimulationEnums.h"
#include "suids.hpp"

namespace Kernel
{
    struct INodeContext;

    struct IMigrate : public ISupports
    {
        virtual void ImmigrateTo(INodeContext* node) = 0;
        virtual void SetMigrating( suids::suid destination, 
                                   MigrationType::Enum type, 
                                   float timeUntilTrip, 
                                   float timeAtDestination,
                                   bool isDestinationNewHome ) = 0;

        virtual const suids::suid & GetMigrationDestination() = 0;
        virtual MigrationType::Enum GetMigrationType() const = 0 ;

        virtual ~IMigrate() {}
    };
}
