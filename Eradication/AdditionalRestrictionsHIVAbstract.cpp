
#include "stdafx.h"

#include "AdditionalRestrictionsHIVAbstract.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHumanHIV.h"

namespace Kernel
{
    AdditionalRestrictionsHIVAbstract::AdditionalRestrictionsHIVAbstract()
    {
        initSimTypes(1, "HIV_SIM");
    }

    IIndividualHumanHIV * AdditionalRestrictionsHIVAbstract::GetHIVIndividual(IIndividualHumanEventContext* pContext) const
    {
        IIndividualHumanHIV * hiv_individual = nullptr;
        if (pContext->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_individual) != s_OK)
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IIndividualHumanEventContext");
        }

        return hiv_individual;
    }

    IIndividualHumanHIV * AdditionalRestrictionsHIVAbstract::GetHIVIndividual(IIndividualHumanSTI* pContext) const
    {
        IIndividualHumanHIV * hiv_individual = nullptr;
        if (pContext->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_individual) != s_OK)
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IIndividualHumanSTI");
        }

        return hiv_individual;

    }
}
