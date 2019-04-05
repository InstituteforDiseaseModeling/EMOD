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
    public:
        GET_SCHEMA_STATIC_WRAPPER( EventTriggerCoordinatorFactory )

        ~EventTriggerCoordinatorFactory();

    private:
        template<class Trigger, class Factory> friend class Kernel::BaseEventTriggerFactory;

        EventTriggerCoordinatorFactory();
    };
}
