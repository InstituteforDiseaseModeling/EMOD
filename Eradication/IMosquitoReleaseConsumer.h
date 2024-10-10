
#pragma once

#include <string>

#include "ISupports.h"
#include "VectorGenome.h"
#include "Types.h"

namespace Kernel
{
    struct IMosquitoReleaseConsumer : public ISupports
    {
        virtual void ReleaseMosquitoes( const std::string&  releasedSpecies,
                                        const VectorGenome& rGenome,
                                        const VectorGenome& rMateGenome,
                                        bool     isRatio,
                                        uint32_t releasedNumber,
                                        float    releasedRatio,
                                        float    releasedInfectious ) = 0;
    };
}

