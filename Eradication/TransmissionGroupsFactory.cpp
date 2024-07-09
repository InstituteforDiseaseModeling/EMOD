
#include "stdafx.h"
#include "TransmissionGroupsFactory.h"
#include "StrainAwareTransmissionGroups.h"
#include "RelationshipGroups.h"
#include "Exceptions.h"

namespace Kernel
{
    // NOTE: StrainAwareTransmissionGroupsGP is not created here because NodeVector
    // needs to have a pointer to the concrete object.
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
            groups = (ITransmissionGroups*) _new_ RelationshipGroups();
            break;
#endif
#endif
        default:
            throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "groupsType", groupsType, TransmissionGroupType::pairs::lookup_key( groupsType ) );
        }

        return groups;
    }
}
