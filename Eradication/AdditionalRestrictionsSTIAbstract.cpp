
#include "stdafx.h"

#include "AdditionalRestrictionsSTIAbstract.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanSTI.h"

namespace Kernel
{
    AdditionalRestrictionsSTIAbstract::AdditionalRestrictionsSTIAbstract()
    {
        initSimTypes(2, "STI_SIM", "HIV_SIM");
    }

    IIndividualHumanSTI * AdditionalRestrictionsSTIAbstract::GetSTIIndividual(IIndividualHumanEventContext* pContext) const
    {
        IIndividualHumanSTI * sti_individual = nullptr;
        if (pContext->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_individual) != s_OK)
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanSTI", "IIndividualHumanEventContext");
        }

        return sti_individual;
    }
}
