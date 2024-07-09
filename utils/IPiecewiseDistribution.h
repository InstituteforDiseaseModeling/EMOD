
#pragma once

#include "Types.h"

namespace Kernel
{
    class IPiecewiseDistribution
    {
    public:
        virtual void SetX( NonNegativeFloat x ) = 0;
    };
}