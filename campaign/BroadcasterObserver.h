
#pragma once

#include "ISupports.h"

namespace Kernel
{
    // This file defines a set of interfaces used to interested objects when things happen to
    // other objects.  These set of interfaces support a modified version of the Gang of Four's
    // Observer pattern.  In this patten, we have the following objects:
    // - IEventObserver - This is an object that wants to hear/be notified about things that happen to other objects.
    // - IEventBroadcaster - This is the object that notifies the observers when stuff happens to the entities.
    // - IEntity - This is the object that has had something happen to it i.e. an event
    // - Trigger - This is object defines the event that happened to the entity.
    // Notice that is not quite the Observer pattern described by the Gang of Four.  In our pattern,
    // The "broadcaster" has the job of notifying "observers" when a "trigger" happens to an "entity".

    struct IEventCoordinatorEventContext;
    struct INodeEventContext;
    struct IIndividualHumanEventContext;
    class EventTriggerCoordinator;
    class EventTriggerNode;
    class EventTrigger;

    // Objects that are interested when events are "triggered" on a particular entity.
    // Objects that implement this interface and register with the broadcaster will
    // have this interface called when the registered event occurs.
    template<class IEntity, class Trigger>
    struct IEventObserver : ISupports
    {
        virtual bool notifyOnEvent( IEntity *pEntity, const Trigger& trigger ) = 0;
    };

    // The "broadcaster" has the job of notifying "observers" when a "trigger" happens to an "entity".
    template<class IObserver, class IEntity, class Trigger>
    struct IEventBroadcaster : ISupports
    {
        virtual void RegisterObserver(   IObserver* pObserver, const Trigger& trigger ) = 0;
        virtual void UnregisterObserver( IObserver* pObserver, const Trigger& trigger ) = 0;
        virtual void TriggerObservers(   IEntity*   pEntity,   const Trigger& trigger ) = 0;
        virtual uint64_t GetNumTriggeredEvents() = 0;
        virtual uint64_t GetNumObservedEvents() = 0;
    };

    // There are three sets of observer, broadcaster, entity combinations.  One for:
    // - Event Coordinators
    // - Nodes
    // - Individuals

    struct ICoordinatorEventObserver : IEventObserver<IEventCoordinatorEventContext, EventTriggerCoordinator>
    {
    };
    struct ICoordinatorEventBroadcaster : IEventBroadcaster<ICoordinatorEventObserver, IEventCoordinatorEventContext, EventTriggerCoordinator>
    {
    };

    struct INodeEventObserver : IEventObserver<INodeEventContext, EventTriggerNode>
    {
    };
    struct INodeEventBroadcaster : IEventBroadcaster<INodeEventObserver, INodeEventContext, EventTriggerNode>
    {
    };

    struct IIndividualEventObserver : IEventObserver<IIndividualHumanEventContext, EventTrigger>
    {
    };
    struct IIndividualEventBroadcaster : IEventBroadcaster<IIndividualEventObserver, IIndividualHumanEventContext, EventTrigger>
    {
    };
}