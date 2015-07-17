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

#include "Interventions.h"
#include "SimpleTypemapRegistration.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"
#include "Configure.h"

namespace Kernel
{
    class BirthTriggeredIV : public IIndividualEventObserver, public BaseNodeIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, BirthTriggeredIV, INodeDistributableIntervention)

    public:
        BirthTriggeredIV();
        virtual ~BirthTriggeredIV();
        virtual int AddRef();
        virtual int Release();
        bool Configure( const Configuration* config );

        // INodeDistributableIntervention
        virtual bool Distribute( INodeEventContext *pNodeEventContext, IEventCoordinator2 *pEC );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(INodeEventContext *context);
        virtual void Update(float dt);

        // IIndividualEventObserver
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange );

    protected:
        INodeEventContext* parent;

        float max_duration;
        float duration;
        float demographic_coverage;

        IndividualInterventionConfig actual_intervention_config;

#if USE_JSON_SERIALIZATION
        void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
    private:
        template<class Archive>
        friend void serialize(Archive &ar, BirthTriggeredIV& iv, const unsigned int v);
#endif
    };
}
