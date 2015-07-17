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
#include "InterventionFactory.h"
#include "Configuration.h"
#include "InterventionEnums.h"
#include "Configure.h"

namespace Kernel
{
    struct IIndividualRepellentConsumer; 

    class SimpleIndividualRepellent : public BaseIntervention 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        bool Configure( const Configuration * config );

        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleIndividualRepellent, IDistributableIntervention)

        SimpleIndividualRepellent();
        virtual ~SimpleIndividualRepellent() { }

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

        // IIndividualRepellent
        virtual float GetBlockingRate() const { return current_blockingrate; }
        virtual float GetKillingRate() const { return current_killingrate; }

    protected:
        InterventionDurabilityProfile::Enum durability_time_profile;
        float current_blockingrate;
        float current_killingrate;
        float primary_decay_time_constant;
        float secondary_decay_time_constant;
        IIndividualRepellentConsumer *ihmc; // aka individual or individual vector interventions container

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, SimpleIndividualRepellent& obj, const unsigned int v);
#endif
    };
}
