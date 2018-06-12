/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Vaccine.h"

namespace Kernel
{

     class SimpleBoosterVaccine : public SimpleVaccine
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleBoosterVaccine, IDistributableIntervention)

    public:
        SimpleBoosterVaccine();
        SimpleBoosterVaccine( const SimpleBoosterVaccine& );
        virtual ~SimpleBoosterVaccine();
 
        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;

    protected:
        virtual void ApplyPrimingAndBoostingEffects(IIndividualHumanInterventionsContext *context);

        float prime_effect;
        float boost_effect;
        float boost_threshold;

        DECLARE_SERIALIZABLE(SimpleBoosterVaccine);
    };
}