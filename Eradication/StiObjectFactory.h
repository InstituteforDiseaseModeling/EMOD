
#pragma once
#include "IReport.h"
#include "ISimulation.h"
#include "InterventionEnums.h"

namespace Kernel
{
    struct StiObjectFactory
    {
        static IReport* CreateRelationshipStartReporter(ISimulation* simulation);
        static IReport* CreateRelationshipEndReporter(ISimulation* simulation);
        static IReport* CreateRelationshipConsummatedReporter(ISimulation* simulation);
        static IReport* CreateTransmissionReporter(ISimulation* simulation);
    };
}
