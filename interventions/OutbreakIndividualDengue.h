/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "OutbreakIndividual.h"

namespace Kernel
{
#ifdef ENABLE_DENGUE
    class OutbreakIndividualDengue : public OutbreakIndividual
    {
        std::string dengueStrainId; // [ 256 ];
        //IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(OutbreakIndividualDengue)
        DECLARE_FACTORY_REGISTERED(InterventionFactory, OutbreakIndividualDengue, IDistributableIntervention)

    public:
        OutbreakIndividualDengue();
        virtual ~OutbreakIndividualDengue() { }
        QuickBuilder GetSchema();
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );

        // INodeDistributableIntervention
//        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
    };
#endif
}
