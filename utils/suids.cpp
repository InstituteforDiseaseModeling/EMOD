/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

// suids provides services for creating and manipulating a set of lightweight IDs unique to a simulation (Simulation Unique ID ~ suid)
// in a system distributed on multiple processes

#include "stdafx.h"
#include "suids.hpp"
#include "IArchive.h"

namespace Kernel
{
    namespace suids 
    {
        void suid::serialize( IArchive& ar, suid& id )
        {
            ar.startObject();
                ar.labelElement("id") & id.data;
            ar.endObject();
        }


        distributed_generator::distributed_generator( int _rank, int _numtasks )
        : rank( _rank )
        , numtasks( _numtasks )
        {
            next_suid.data = rank + 1; // +1 ensures that NIL will never be generated
        }

        suid distributed_generator::operator()()
        {
            suid tmp = next_suid;
            next_suid.data += numtasks;
            return tmp;
        }

        void distributed_generator::serialize( IArchive& ar, distributed_generator& generator )
        {
            ar.startObject();
                ar.labelElement("next_suid") & generator.next_suid;
                ar.labelElement("rank")      & generator.rank;
                ar.labelElement("numtasks")  & generator.numtasks;
            ar.endObject();
        }
    }
}
