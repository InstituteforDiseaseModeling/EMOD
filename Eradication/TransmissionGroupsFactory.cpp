/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "TransmissionGroupsFactory.h"
#include "SimpleTransmissionGroups.h"
#include "VectorTransmissionGroups.h"    // includes StrainAwareTransmissionGroups and MultiRouteTransmissionGroups
#include "RelationshipGroups.h"
#include "Exceptions.h"

namespace Kernel
{
    ITransmissionGroups* TransmissionGroupsFactory::CreateNodeGroups(TransmissionGroupType::Enum groupsType)
    {
        ITransmissionGroups* groups = nullptr;

        switch (groupsType)
        {
        case TransmissionGroupType::SimpleGroups:
            groups = (ITransmissionGroups*) _new_ SimpleTransmissionGroups;
            break;

        case TransmissionGroupType::MultiRouteGroups:
            groups = (ITransmissionGroups*) _new_ MultiRouteTransmissionGroups;
            break;

        case TransmissionGroupType::StrainAwareGroups:
            groups = (ITransmissionGroups*) _new_ StrainAwareTransmissionGroups;
            break;
#ifndef DISABLE_VECTOR
        case TransmissionGroupType::HumanVectorGroups:
            groups = (ITransmissionGroups*) _new_ VectorTransmissionGroups;
            break;
#endif
#ifndef DISABLE_HIV
#ifndef _DLLS_
        case TransmissionGroupType::RelationshipGroups:
            groups = (ITransmissionGroups*) _new_ RelationshipGroups;
            break;
#endif
#endif
        default:
            throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "groupsType", groupsType, TransmissionGroupType::pairs::lookup_key( groupsType ) );
        }

        return groups;
    }
}
