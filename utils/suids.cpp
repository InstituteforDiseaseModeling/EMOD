/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

// suids provides services for creating and manipulating a set of lightweight IDs unique to a simulation (Simulation Unique ID ~ suid)
// in a system distributed on multiple processes
// modelled on the boost uuid interface in case we want to extend in that direction eventually
#include "stdafx.h"
#include "suids.hpp"

#include "RapidJsonImpl.h"

namespace Kernel
{
    namespace suids 
    {
        BEGIN_QUERY_INTERFACE_BODY(suid)
        HANDLE_INTERFACE(IJsonSerializable)
        END_QUERY_INTERFACE_BODY(suid)

#if USE_JSON_SERIALIZATION
        void suid::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
        {
            root->BeginObject();
            root->Insert("data", data);
            root->EndObject();
        }

        void suid::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
        {
            data = root->GetInt("data");
        }
#endif
    }
}
