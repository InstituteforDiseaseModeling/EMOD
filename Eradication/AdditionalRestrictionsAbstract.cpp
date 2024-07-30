
#include "stdafx.h"

#include "AdditionalRestrictionsAbstract.h"

namespace Kernel
{
    AdditionalRestrictionsAbstract::AdditionalRestrictionsAbstract() 
        : JsonConfigurable()
        , m_CompareTo(true)
    {
    }

    bool AdditionalRestrictionsAbstract::Configure(const Configuration* config)
    {
        initConfigTypeMap("Is_Equal_To", &m_CompareTo, AR_Is_Equal_To_DESC_TEXT, true);

        return JsonConfigurable::Configure(config);
    }
}