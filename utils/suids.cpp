/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

// suids provides services for creating and manipulating a set of lightweight IDs unique to a simulation (Simulation Unique ID ~ suid)
// in a system distributed on multiple processes
// modelled on the boost uuid interface in case we want to extend in that direction eventually
#include "stdafx.h"
#include "suids.hpp"
#include "IArchive.h"

namespace Kernel
{
    namespace suids 
    {
        BEGIN_QUERY_INTERFACE_BODY(suid)
        END_QUERY_INTERFACE_BODY(suid)

        void suid::serialize( IArchive& ar, suid& id )
        {
            ar.labelElement("id") & id.data;
        }
    }
}
