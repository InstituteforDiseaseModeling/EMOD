/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IReport.h"
#include "ISimulation.h"

namespace Kernel
{
    struct HivObjectFactory
    {
        static IReport* CreateRelationshipStartReporter(ISimulation* simulation);
        static IReport* CreateTransmissionReporter(ISimulation* simulation);
        static IReport* CreateHIVMortalityReporter(ISimulation* simulation);
        static IReport* CreateHIVByAgeAndGenderReporter(ISimulation* simulation, float hivPeriod );
        static IReport* CreateHIVInfectionReporter(ISimulation* simulation);
        static IReport* CreateHIVARTReporter(ISimulation* simulation);
    };
}
