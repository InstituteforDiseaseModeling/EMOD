
#pragma once

#include <string>

namespace Kernel
{
    class VectorGenome;

    struct IVectorGenomeNames
    {
        virtual std::string GetGenomeName( const VectorGenome& rGenome ) const = 0;
    };
}
