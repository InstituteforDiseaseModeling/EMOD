/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "HivObjectFactory.h"
#include "ReportHIVMortalityEvents.h"
#include "ReportHIVByAgeAndGender.h"
#include "ReportHIVInfection.h"
#include "ReportHIVART.h"
#include "HIVRelationshipStartReporter.h"
#include "HIVTransmissionReporter.h"

namespace Kernel
{
    IReport* HivObjectFactory::CreateRelationshipStartReporter(ISimulation* simulation)
    {
        return HIVRelationshipStartReporter::Create(simulation);
    }

    IReport* HivObjectFactory::CreateTransmissionReporter(ISimulation* simulation)
    {
        return HIVTransmissionReporter::Create(simulation);
    }

    IReport* HivObjectFactory::CreateHIVMortalityReporter(ISimulation* simulation)
    {
        return ReportHIVMortalityEvents::Create(simulation);
    }

    IReport* HivObjectFactory::CreateHIVByAgeAndGenderReporter(ISimulation* simulation, float hivPeriod )
    {
        return ReportHIVByAgeAndGender::Create(simulation,hivPeriod);
    }

    IReport* HivObjectFactory::CreateHIVInfectionReporter(ISimulation* simulation)
    {
        return ReportHIVInfection::Create(simulation);
    }

    IReport* HivObjectFactory::CreateHIVARTReporter(ISimulation* simulation)
    {
        return ReportHIVART::Create(simulation);
    }
}
