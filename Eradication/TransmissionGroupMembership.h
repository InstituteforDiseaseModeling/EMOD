/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>

using namespace std;

namespace Kernel
{
    typedef int32_t GroupIndex;

    struct TransmissionGroupMembership_t
    {
        TransmissionGroupMembership_t() : group( 0x0BADF00D ) {}
        TransmissionGroupMembership_t( int32_t init ) : group(init) {}
        GroupIndex group;
    };
}
