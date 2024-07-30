
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
