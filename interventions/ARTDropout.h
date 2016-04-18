/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
        bool Configure( const Configuration * );
        ARTDropout();
        virtual ~ARTDropout();

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );

        virtual std::string GetDrugName();

    protected:
        // These have same names as analogous methods on container but are internal for the drug itself.

        virtual void ApplyEffects();

        IHIVDrugEffectsApply * itbda;

        DECLARE_SERIALIZABLE(ARTDropout);
    };
}
