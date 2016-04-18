/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"

#include "MalariaContexts.h"  // for MalariaAntibodyType enum

namespace Kernel
{
    class RTSSVaccine : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, RTSSVaccine, IDistributableIntervention)

    public:
        RTSSVaccine();
        virtual ~RTSSVaccine() { }

        bool Configure( const Configuration * config );

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

    protected:
        MalariaAntibodyType::Enum antibody_type;
        int antibody_variant;
        float boosted_antibody_concentration;

        DECLARE_SERIALIZABLE(RTSSVaccine);
    };
}
