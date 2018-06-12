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
    class ARTBasic : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ARTBasic, IDistributableIntervention);

    public:
        ARTBasic();
        virtual ~ARTBasic();

        virtual bool Configure( const Configuration * ) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void Update( float dt ) override;

    protected:
        // These have same names as analogous methods on container but are internal for the drug itself.

        bool viral_suppression;
        float days_to_achieve_suppression ;

        DECLARE_SERIALIZABLE(ARTBasic);
    };
}
