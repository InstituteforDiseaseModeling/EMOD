/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Drugs.h"

namespace Kernel
{
    struct IHIVDrugEffectsApply;

    class ARTDropout : public GenericDrug
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ARTDropout, IDistributableIntervention);

    public:
        ARTDropout();
        virtual ~ARTDropout();

        virtual bool Configure( const Configuration * ) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;

        virtual std::string GetDrugName() const override;

    protected:
        // These have same names as analogous methods on container but are internal for the drug itself.

        virtual void ApplyEffects() override;

        IHIVDrugEffectsApply * itbda;

        DECLARE_SERIALIZABLE(ARTDropout);
    };
}
