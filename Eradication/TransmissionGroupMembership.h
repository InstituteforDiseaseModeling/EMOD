
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
