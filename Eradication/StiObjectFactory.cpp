
#include "stdafx.h"

#include "StiObjectFactory.h"
#include "ISimulation.h"
#include "StiRelationshipStartReporter.h"
#include "StiRelationshipEndReporter.h"
#include "StiRelationshipConsummatedReporter.h"
#include "StiTransmissionReporter.h"

namespace Kernel
{
    IReport* StiObjectFactory::CreateRelationshipStartReporter(ISimulation* simulation)
    {
        return StiRelationshipStartReporter::Create(simulation);
    }

    IReport* StiObjectFactory::CreateRelationshipEndReporter(ISimulation* simulation)
    {
        return StiRelationshipEndReporter::Create(simulation);
    }

    IReport* StiObjectFactory::CreateRelationshipConsummatedReporter(ISimulation* simulation)
    {
        return StiRelationshipConsummatedReporter::Create(simulation);
    }

    IReport* StiObjectFactory::CreateTransmissionReporter(ISimulation* simulation)
    {
        return StiTransmissionReporter::Create(simulation);
    }

}
