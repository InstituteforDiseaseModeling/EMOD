
#pragma once

#include "BaseEventTrigger.h"

namespace Kernel
{
    class EventTriggerInternal;
    class EventTriggerCoordinatorFactory;
    class EventTriggerCoordinator;

    // An EventTriggerCoordinator is an event for an IEventCoodinator.
    // Coordinators need events as a way to communicate with other coordinators.
    class EventTriggerCoordinator : public BaseEventTrigger<EventTriggerCoordinator,EventTriggerCoordinatorFactory>
    {
    public:
        EventTriggerCoordinator();
        explicit EventTriggerCoordinator( const std::string &init_str );
        explicit EventTriggerCoordinator( const char *init_str );
        ~EventTriggerCoordinator();

    protected:
        template<class Trigger, class Factory> friend class Kernel::BaseEventTriggerFactory;

        explicit EventTriggerCoordinator( EventTriggerInternal* peti );
    };

    class EventTriggerCoordinatorFactory : public BaseEventTriggerFactory<EventTriggerCoordinator, EventTriggerCoordinatorFactory>
    {
        GET_SCHEMA_STATIC_WRAPPER( EventTriggerCoordinatorFactory )
    public:

        virtual ~EventTriggerCoordinatorFactory();

    private:
        template<class Trigger, class Factory> friend class Kernel::BaseEventTriggerFactory;

        EventTriggerCoordinatorFactory();
    };
}
