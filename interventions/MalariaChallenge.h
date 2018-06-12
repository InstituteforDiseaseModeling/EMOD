/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "EventCoordinator.h"
#include "Configure.h"

namespace Kernel
{
    ENUM_DEFINE(MalariaChallengeType,
        ENUM_VALUE_SPEC(InfectiousBites         , 1)
        ENUM_VALUE_SPEC(Sporozoites             , 2))

    class MalariaChallenge : public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(Outbreak)
        DECLARE_FACTORY_REGISTERED(InterventionFactory, MalariaChallenge, INodeDistributableIntervention)

    public:
        MalariaChallenge();
        MalariaChallenge( const MalariaChallenge& master );
        virtual ~MalariaChallenge() { }

        // INodeDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC);
        virtual void SetContextTo(INodeEventContext *context) { /* not needed for this intervention */ }
        virtual void Update(float dt);

    protected:
        MalariaChallengeType::Enum challenge_type;
        int n_challenged_objects;
        float coverage;
    };
}
