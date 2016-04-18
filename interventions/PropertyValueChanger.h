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

namespace Kernel
{
    struct IPropertyValueChangerEffects;
    /* Keep around as an identity solution??? */
    struct IPropertyValueChanger : public ISupports
    {
        virtual const char * GetTargetPropertyValue() = 0;
    };

    class PropertyValueChanger : public BaseIntervention, public IPropertyValueChanger
    {
        public:
        bool Configure( const Configuration * config );

        DECLARE_FACTORY_REGISTERED(InterventionFactory, PropertyValueChanger, IDistributableIntervention)

    public:
        PropertyValueChanger();

        // factory method
        virtual ~PropertyValueChanger();

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

        virtual int AddRef();
        virtual int Release();

        virtual const char * GetTargetPropertyValue();

    protected:
        IIndividualHumanContext *parent;
        jsonConfigurable::ConstrainedString target_property_key;
        jsonConfigurable::ConstrainedString target_property_value;
        IPropertyValueChangerEffects *ibc;
        float probability;
        float revert;
        float max_duration;
        float action_timer;
        float reversion_timer;

        DECLARE_SERIALIZABLE(PropertyValueChanger);
    };
}
