/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>

#include "Interventions.h"
#include "InterventionFactory.h"
#include "Configuration.h"
#include "InterventionEnums.h"
#include "Configure.h"
#include "IWaningEffect.h"

namespace Kernel
{
    struct IHousingModificationConsumer;

    class SimpleHousingModification : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleHousingModification, IDistributableIntervention)

    public:
        virtual bool Configure( const Configuration * config ) override;
        SimpleHousingModification();
        SimpleHousingModification( const SimpleHousingModification& );
        virtual ~SimpleHousingModification();

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;
        virtual void Update(float dt) override;

    protected:
        IWaningEffect* killing_effect;
        IWaningEffect* blocking_effect;
        IHousingModificationConsumer *ihmc; // aka individual or individual vector interventions container

        DECLARE_SERIALIZABLE(SimpleHousingModification);
    };

    class IRSHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, IRSHousingModification, IDistributableIntervention)

        DECLARE_SERIALIZABLE(IRSHousingModification);
    };

    class ScreeningHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ScreeningHousingModification, IDistributableIntervention)

        DECLARE_SERIALIZABLE(ScreeningHousingModification);
    };

    class SpatialRepellentHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SpatialRepellentHousingModification, IDistributableIntervention)

        DECLARE_SERIALIZABLE(SpatialRepellentHousingModification);
    };

    class ArtificialDietHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ArtificialDietHousingModification, IDistributableIntervention)

        DECLARE_SERIALIZABLE(ArtificialDietHousingModification);
    };

    class InsectKillingFenceHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, InsectKillingFenceHousingModification, IDistributableIntervention)

        DECLARE_SERIALIZABLE(InsectKillingFenceHousingModification);
    };
}
