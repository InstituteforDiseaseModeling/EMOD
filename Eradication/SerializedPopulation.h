/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <cstdint>

namespace Kernel{
    struct ISimulation;
    class Simulation;
}

namespace SerializedState {
    Kernel::ISimulation* LoadSerializedSimulation(const char* filename);
    void SaveSerializedSimulation(Kernel::Simulation* sim, uint32_t time_step, bool compress);

    // Declare these so the friend declarations in Simulation.h will work:
    struct Header;
    Kernel::ISimulation* ReadDtkVersion2(FILE* f, const char* filename, Header& header);
    Kernel::ISimulation* ReadDtkVersion34(FILE* f, const char* filename, Header& header);
}
