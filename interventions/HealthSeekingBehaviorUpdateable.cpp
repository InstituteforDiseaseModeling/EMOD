/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "HealthSeekingBehaviorUpdateable.h"

static const char * _module = "HealthSeekingBehaviorUpdateable";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HealthSeekingBehaviorUpdateable, SimpleHealthSeekingBehavior)
        HANDLE_INTERFACE(IHealthSeekingBehavior)
    END_QUERY_INTERFACE_DERIVED(HealthSeekingBehaviorUpdateable, SimpleHealthSeekingBehavior)

    IMPLEMENT_FACTORY_REGISTERED(HealthSeekingBehaviorUpdateable)

    HealthSeekingBehaviorUpdateable::HealthSeekingBehaviorUpdateable() : SimpleHealthSeekingBehavior()
    {
        initSimTypes( 1, "TB_SIM" );
    }

    void HealthSeekingBehaviorUpdateable::UpdateProbabilityofSeeking( float new_probability_of_seeking )
    {
        LOG_DEBUG_F("old probability of seeking is %f, new probability of seeking is %f\n", probability_of_seeking, new_probability_of_seeking);
        probability_of_seeking = new_probability_of_seeking;
    }

    
    
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::HealthSeekingBehaviorUpdateable)

namespace Kernel {
    // Would like to put all serialization stuff here but having these in headers
    // is posing a problem due to multiple subclasses. :(
    //REGISTER_SERIALIZATION_VOID_CAST(SimpleHealthSeekingBehaviour, IDistributableIntervention);

    template<class Archive>
    void serialize(Archive &ar, HealthSeekingBehaviorUpdateable& obj, const unsigned int v)
    {
        static const char * _module = "HealthSeekingBehaviorUpdateable";
        LOG_DEBUG("(De)serializing HealthSeekingBehaviorUpdateable\n");

        boost::serialization::void_cast_register<HealthSeekingBehaviorUpdateable, IDistributableIntervention>();

        ar & boost::serialization::base_object<Kernel::SimpleHealthSeekingBehavior>(obj);
    }
}
#endif
