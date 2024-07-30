
#pragma once

#include "ISupports.h"
#include "IStrainIdentity.h"
#include "GeneticProbability.h"

namespace Kernel
{
    struct IContagionPopulationGP : public ISupports
    {
        virtual const GeneticProbability& GetTotalContagionGP() const = 0;
    };
}
