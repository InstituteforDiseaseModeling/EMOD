/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Interventions.h"
#include "InterventionFactory.h"    // macros that 'auto'-register classes

namespace Kernel
{
    struct IHIVDrugEffectsApply;

    class ARTDropout : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ARTDropout, IDistributableIntervention);

    public:
        ARTDropout();
        virtual ~ARTDropout();

        virtual bool Configure( const Configuration * ) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void Update( float dt ) override;

    protected:

        DECLARE_SERIALIZABLE(ARTDropout);
    };
}
