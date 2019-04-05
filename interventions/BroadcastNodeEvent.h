/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>

#include "Configuration.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "EventTriggerNode.h"

namespace Kernel
{
    class BroadcastNodeEvent : public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED( InterventionFactory, BroadcastNodeEvent, INodeDistributableIntervention )

    public:
        BroadcastNodeEvent();
        BroadcastNodeEvent( const BroadcastNodeEvent& master );
        virtual ~BroadcastNodeEvent();

        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual void Update( float dt ) override;

    protected:
        EventTriggerNode m_EventToBroadcast;

        // TODO - can't do until BaseNodeIntervention is serializable
        //DECLARE_SERIALIZABLE(BroadcastNodeEvent);
    };
}
