/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "Configuration.h"
#include "Configure.h"
#include "Contexts.h"
#include "HealthSeekingBehavior.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "SimpleTypemapRegistration.h"

namespace Kernel
{
    class IDMAPI BroadcastEvent :  public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, BroadcastEvent, IDistributableIntervention)

    public: 
        BroadcastEvent();
        BroadcastEvent( const BroadcastEvent& master );
        virtual ~BroadcastEvent() {  }
        bool Configure( const Configuration* pConfig );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context) { parent = context; } // for rng
        virtual void Update(float dt);

    protected:

        void broadcastEvent(const std::string& event);

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        IIndividualHumanContext *parent;
        ConstrainedString broadcast_event;
#pragma warning( pop )

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, BroadcastEvent &obj, const unsigned int v);
#endif
    };
}
