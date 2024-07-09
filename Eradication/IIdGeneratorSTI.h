
#pragma once

#include "ISupports.h"
#include "suids.hpp"

namespace Kernel
{
    struct IIdGeneratorSTI : virtual ISupports
    {
        virtual suids::suid GetNextRelationshipSuid() = 0;
    };
}
