/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "ITransmissionGroups.h"
#include "IntranodeTransmissionTypes.h"

namespace Kernel
{
    class TransmissionGroupsFactory
    {
    public:
        static ITransmissionGroups* CreateNodeGroups(TransmissionGroupType::Enum groupsType);
    };
}