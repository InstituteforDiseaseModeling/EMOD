
#pragma once

#include <string>

namespace Kernel
{
    struct IIndividualHumanEventContext;
    struct IRelationship;
}

namespace ReportUtilitiesSTI
{
    std::string GetRelationshipTypeColumnHeader();
    Kernel::IRelationship* GetTransmittingRelationship( Kernel::IIndividualHumanEventContext* recipientContext );
}
