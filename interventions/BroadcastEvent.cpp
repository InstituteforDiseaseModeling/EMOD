/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "BroadcastEvent.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "SimulationConfig.h"  // for listed_events

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
        broadcast_event.constraints = "<configuration>:Listed_Events.*";
        broadcast_event.constraint_param = &GET_CONFIGURABLE(SimulationConfig)->listed_events;
        initConfigTypeMap( "Broadcast_Event", &broadcast_event, HIV_Broadcast_Event_DESC_TEXT, NO_TRIGGER_STR );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( broadcast_event == NO_TRIGGER_STR )
        {
            LOG_WARN_F("BroadcastEvent was configured with NoTrigger as the Broadcast_Event.  This special event will not be broadcast.");
        }
        return ret;
    }

    BroadcastEvent::BroadcastEvent()
    : broadcast_event( NO_TRIGGER_STR )
    {
    }

    BroadcastEvent::BroadcastEvent( const BroadcastEvent& master )
        :BaseIntervention( master )
    {
        broadcast_event = master.broadcast_event;
    }

    void BroadcastEvent::Update( float dt )
    {
        // broadcast the event
        INodeTriggeredInterventionConsumer* broadcaster = nullptr;
        if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
        }
        broadcaster->TriggerNodeEventObserversByString( parent->GetEventContext(), broadcast_event );

        // expire the intervention
        expired = true;
    }

    void 
    BroadcastEvent::broadcastEvent(const std::string& event)
    {
        if( event != NO_TRIGGER_STR )
        {
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
            LOG_DEBUG_F( "BroadcastEvent broadcasting event = %s.\n", broadcast_event.c_str() );
            broadcaster->TriggerNodeEventObserversByString( parent->GetEventContext(), event );
        }
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::BroadcastEvent)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, BroadcastEvent& obj, const unsigned int v)
    {
        static const char * _module = "BroadcastEvent";
        LOG_DEBUG("(De)serializing BroadcastEvent\n");

        boost::serialization::void_cast_register<BroadcastEvent, IDistributableIntervention>();
        ar & (std::string) obj.broadcast_event;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(obj);
        //ar & boost::serialization::base_object<Kernel::SimpleHealthSeekingBehavior>(obj);
    }
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::BroadcastEvent&, unsigned int);
}
#endif
