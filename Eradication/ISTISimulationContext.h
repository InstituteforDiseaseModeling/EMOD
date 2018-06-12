/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "suids.hpp"
#include "ISupports.h"

namespace Kernel
{
    struct ISTISimulationContext : virtual ISupports
    {
        virtual void AddTerminatedRelationship( const suids::suid& nodeSuid, const suids::suid& relId ) = 0;
        virtual bool WasRelationshipTerminatedLastTimestep( const suids::suid& relId ) const = 0;
    };
}
