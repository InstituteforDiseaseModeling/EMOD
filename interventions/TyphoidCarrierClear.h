/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "EventCoordinator.h"
#include "Configure.h"

namespace Kernel
{
    class TyphoidCarrierClear : public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, TyphoidCarrierClear, IDistributableIntervention)

    public:
        TyphoidCarrierClear();
        virtual ~TyphoidCarrierClear();
        virtual bool Configure( const Configuration* pConfig ) override;
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual void SetContextTo(IIndividualHumanContext *context) { /* not needed for this intervention */ }
        virtual void Update(float dt);

    protected:
        float clearance_rate;
        //DECLARE_SERIALIZABLE(TyphoidCarrierClear);
    };
}

