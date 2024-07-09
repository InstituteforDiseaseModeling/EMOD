
#pragma once

#include "AdditionalRestrictionsSTIAbstract.h"

namespace Kernel
{
    struct IIndividualHumanHIV;

    class AdditionalRestrictionsHIVAbstract : public AdditionalRestrictionsSTIAbstract
    {
    public:
        AdditionalRestrictionsHIVAbstract();

    protected:
        IIndividualHumanHIV * GetHIVIndividual(IIndividualHumanEventContext* pContext) const;
        IIndividualHumanHIV * GetHIVIndividual(IIndividualHumanSTI* pContext) const;
    };
}
