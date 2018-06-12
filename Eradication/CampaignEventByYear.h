/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "CampaignEvent.h"

namespace Kernel
{
    class CampaignEventByYear : public CampaignEvent
    {
        DECLARE_FACTORY_REGISTERED(CampaignEventFactory, CampaignEventByYear, IConfigurable)

    public:
        friend class CampaignEventFactory;
        DECLARE_CONFIGURED(CampaignEventByYear)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

        CampaignEventByYear();
        virtual ~CampaignEventByYear();
    };
}
