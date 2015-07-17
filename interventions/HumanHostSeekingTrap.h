/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "SimpleTypemapRegistration.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "Configure.h"

namespace Kernel
{
    struct IVectorInterventionEffectsSetter; 

    /* Keep around as an identity solution??? */
    struct IHumanHostSeekingTrap : public ISupports
    {
    };

    class HumanHostSeekingTrap : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HumanHostSeekingTrap, IDistributableIntervention)

    public:
        bool Configure( const Configuration * config );
        HumanHostSeekingTrap();
        virtual ~HumanHostSeekingTrap() { }

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

    protected:
        float current_attractrate;
        float current_killingrate;
        float primary_decay_time_constant;
        float secondary_decay_time_constant;
        InterventionDurabilityProfile::Enum durability_time_profile;
        IVectorInterventionEffectsSetter *ivies;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<typename Archive>
        friend void serialize( Archive &ar, HumanHostSeekingTrap& obj, unsigned int version );
#endif    
    };
}
