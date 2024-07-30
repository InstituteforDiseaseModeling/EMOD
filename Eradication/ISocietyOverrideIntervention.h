
#pragma once

#include "ISupports.h"
#include "RelationshipType.h"

namespace Kernel
{
    // Node-level interventions that modify parameters within Society
    // need to implement this interface.  This will allow their addition
    // to the node to be staged and added at the end of the node being
    // updated.  See NodeSTIEventContextHost::ApplyStagedInterventions()
    // for more information.
    struct ISocietyOverrideIntervention : ISupports
    {
        virtual RelationshipType::Enum GetRelationshipType() const = 0;
    };
}
