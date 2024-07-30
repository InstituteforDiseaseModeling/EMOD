
#pragma once

#include "ISerializable.h"

namespace Kernel
{
    struct IIndividualHumanEventContext;

    // This is an interface to restriction objects that are used to 
    // qualify individuals for intervention targeting.
    struct IAdditionalRestrictions : public ISerializable
    {
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const = 0;
    };
}