/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
