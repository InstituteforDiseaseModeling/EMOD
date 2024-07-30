
#pragma once

#include "ISimulation.h"

namespace Kernel
{
    class Simulation;

    class SimulationFactory
    {
    public:
        static ISimulation* CreateSimulation();
    };
}
