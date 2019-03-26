/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "TransmissionGroupsFactory.h"
#include "StrainAwareTransmissionGroups.h"
#include "RelationshipGroups.h"
#include "Exceptions.h"

namespace Kernel
{
    ITransmissionGroups* TransmissionGroupsFactory::CreateNodeGroups( TransmissionGroupType::Enum groupsType, RANDOMBASE* prng )
    {
        ITransmissionGroups* groups = nullptr;

        switch (groupsType)
        {
        case TransmissionGroupType::StrainAwareGroups:
            groups = (ITransmissionGroups*) _new_ StrainAwareTransmissionGroups( prng );
            break; 
#ifndef DISABLE_HIV
#ifndef _DLLS_
        case TransmissionGroupType::RelationshipGroups:
            groups = (ITransmissionGroups*) _new_ RelationshipGroups( prng );
            break;
#endif
#endif
        default:
            throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "groupsType", groupsType, TransmissionGroupType::pairs::lookup_key( groupsType ) );
        }

        return groups;
    }
}
