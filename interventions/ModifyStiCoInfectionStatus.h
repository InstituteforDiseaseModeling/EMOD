/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>

#include "Interventions.h"
#include "InterventionFactory.h"

namespace Kernel
{
    class ModifyStiCoInfectionStatus : public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(ModifyStiCoInfectionStatus)
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ModifyStiCoInfectionStatus, IDistributableIntervention)
        DECLARE_QUERY_INTERFACE()

    public:
        ModifyStiCoInfectionStatus();
        virtual ~ModifyStiCoInfectionStatus() { }

        // INodeDistributableIntervention
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void Update(float dt) override;

    protected:
        bool set_flag_to;

        DECLARE_SERIALIZABLE(ModifyStiCoInfectionStatus);
    };
}
