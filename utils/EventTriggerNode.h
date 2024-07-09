
#pragma once

#include "BaseEventTrigger.h"

namespace Kernel
{
    class EventTriggerInternal;
    class EventTriggerNodeFactory;
    class EventTriggerNode;

    // An EventTriggerNode is an event that is related to an INodeEventContext.
    // Nodes need events as a way to communicate with other nodes.
    class EventTriggerNode : public BaseEventTrigger<EventTriggerNode,EventTriggerNodeFactory>
    {
    public:
        //static const EventTriggerNode& SheddingComplete;

        EventTriggerNode();
        explicit EventTriggerNode( const std::string &init_str );
        explicit EventTriggerNode( const char *init_str );
        ~EventTriggerNode();

    protected:
        template<class Trigger, class Factory> friend class Kernel::BaseEventTriggerFactory;

        explicit EventTriggerNode( EventTriggerInternal* peti );
    };

    class EventTriggerNodeFactory : public BaseEventTriggerFactory<EventTriggerNode, EventTriggerNodeFactory>
    {
        GET_SCHEMA_STATIC_WRAPPER( EventTriggerNodeFactory )
    public:

        virtual ~EventTriggerNodeFactory();

    private:
        template<class Trigger, class Factory> friend class Kernel::BaseEventTriggerFactory;

        EventTriggerNodeFactory();
    };
}
