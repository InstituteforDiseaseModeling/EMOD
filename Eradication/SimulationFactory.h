/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "ISimulation.h"
#include <list>

using namespace std;

namespace Kernel
{
    class Simulation;

    class SimulationFactory
    {
    public:
        static ISimulation* CreateSimulation();

        template<class IArchiveT, class SimulationT>
        static SimulationT* CreateSimulationFromArchive(IArchiveT &ia);
    };

    template<class IArchiveT, class SimulationT>
    SimulationT* SimulationFactory::CreateSimulationFromArchive( IArchiveT &ia )
    {
#ifdef _DLLS_
        ISimulation *sim = SimulationT::CreateSimulation();
#else
        SimulationT *sim = SimulationT::CreateSimulation();
#endif
        ia >> (*sim);
        return sim;
    }
}
