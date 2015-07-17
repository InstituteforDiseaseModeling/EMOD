/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "HealthSeekingBehavior.h"
#include "IHealthSeekingBehavior.h"


namespace Kernel
{

    class HealthSeekingBehaviorUpdateable :  public IHealthSeekingBehavior, public SimpleHealthSeekingBehavior
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HealthSeekingBehaviorUpdateable, IDistributableIntervention)
    
    public: 
        HealthSeekingBehaviorUpdateable();
        virtual ~HealthSeekingBehaviorUpdateable() {};

        //IHealthSeekingBehavior
        virtual void UpdateProbabilityofSeeking(float new_probability_of_seeking); //this function only called by TBInterventions Container

    protected:

    private:
        // Serialization
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        template<class Archive>
        friend void serialize(Archive &ar, HealthSeekingBehaviorUpdateable &obj, const unsigned int v);
#endif
    };
}
