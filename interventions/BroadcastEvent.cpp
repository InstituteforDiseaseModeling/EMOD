/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "BroadcastEvent.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)

static const char * _module = "BroadcastEvent";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(BroadcastEvent)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(BroadcastEvent)

    IMPLEMENT_FACTORY_REGISTERED(BroadcastEvent)

    bool BroadcastEvent::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Broadcast_Event", &broadcast_event, HIV_Broadcast_Event_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( (broadcast_event == NO_TRIGGER_STR) || broadcast_event.IsUninitialized() )
        {
            LOG_WARN_F("BroadcastEvent was configured with NoTrigger (or uninitialized) as the Broadcast_Event.  This special event will not be broadcast.\n");
        }
        return ret;
    }

    BroadcastEvent::BroadcastEvent()
    : broadcast_event()
    {
    }

    BroadcastEvent::BroadcastEvent( const BroadcastEvent& master )
        :BaseIntervention( master )
    {
        broadcast_event = master.broadcast_event;
    }

    void BroadcastEvent::Update( float dt )
    {
        if( (broadcast_event != NO_TRIGGER_STR) && !broadcast_event.IsUninitialized() )
        {
            // broadcast the event
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
            broadcaster->TriggerNodeEventObserversByString( parent->GetEventContext(), broadcast_event );
        }

        // expire the intervention
        expired = true;
    }

    REGISTER_SERIALIZABLE(BroadcastEvent);

    void BroadcastEvent::serialize(IArchive& ar, BroadcastEvent* obj)
    {
        BaseIntervention::serialize( ar, obj );
        BroadcastEvent& be = *obj;
        ar.labelElement("broadcast_event") & be.broadcast_event;
    }
}
