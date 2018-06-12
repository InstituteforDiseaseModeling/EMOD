/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "OutbreakIndividual.h"
#include "StrainIdentity.h"

namespace Kernel
{
    class IndividualHumanCoInfection;
    class OutbreakIndividualTBorHIV : public OutbreakIndividual
    {

        DECLARE_CONFIGURED(OutbreakIndividualTBorHIV)
        DECLARE_FACTORY_REGISTERED(InterventionFactory, OutbreakIndividualTBorHIV, IDistributableIntervention)
    protected:
        int infection_type;
    public:
        OutbreakIndividualTBorHIV();
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual ~OutbreakIndividualTBorHIV() { }
        QuickBuilder GetSchema();
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO);
    };
}
