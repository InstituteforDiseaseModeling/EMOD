/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
        static const EventTriggerNode& SheddingComplete;

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
    public:
        GET_SCHEMA_STATIC_WRAPPER( EventTriggerNodeFactory )

        ~EventTriggerNodeFactory();

    private:
        template<class Trigger, class Factory> friend class Kernel::BaseEventTriggerFactory;

        EventTriggerNodeFactory();
    };
}
