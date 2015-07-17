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
    struct IHousingModificationConsumer;

    class SimpleHousingModification : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleHousingModification, IDistributableIntervention)

    public:
        bool Configure( const Configuration * config );
        SimpleHousingModification();
        virtual ~SimpleHousingModification() { }

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

    protected:
        InterventionDurabilityProfile::Enum durability_time_profile;
        float current_blockingrate;
        float current_killingrate;
        float primary_decay_time_constant;
        float secondary_decay_time_constant;
        IHousingModificationConsumer *ihmc; // aka individual or individual vector interventions container

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, SimpleHousingModification& hm, const unsigned int v);
#endif
    };

    class IRSHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, IRSHousingModification, IDistributableIntervention)

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, IRSHousingModification& hm, const unsigned int v);
#endif
    };

    class ScreeningHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ScreeningHousingModification, IDistributableIntervention)

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, ScreeningHousingModification& hm, const unsigned int v);
#endif
    };

    class SpatialRepellentHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SpatialRepellentHousingModification, IDistributableIntervention)

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, SpatialRepellentHousingModification& hm, const unsigned int v);
#endif
    };

    class ArtificialDietHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ArtificialDietHousingModification, IDistributableIntervention)

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, ArtificialDietHousingModification& hm, const unsigned int v);
#endif
    };

    class InsectKillingFenceHousingModification : public SimpleHousingModification
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, InsectKillingFenceHousingModification, IDistributableIntervention)

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, InsectKillingFenceHousingModification& hm, const unsigned int v);
#endif
    };
}
