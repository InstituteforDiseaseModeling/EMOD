
#pragma once

#include <vector>
#include "BroadcasterImpl.h"
#include "Log.h"
#include "Exceptions.h"

namespace Kernel
{
    // This file contains the implementation for the BroadcasterImpl.  We want to reduce the number
    // of files that contain this implementation so that when we make changes we do not compile a
    // ton files.

    template<class Observer, class Entity, class Trigger, class TriggerFactory>
    BroadcasterImpl<Observer,Entity,Trigger,TriggerFactory>::BroadcasterImpl()
        : observers()
        , disposed_observers()
        , num_triggered_events(0)
        , num_observed_events(0)
    {
        int num_triggers = TriggerFactory::GetInstance()->GetNumEventTriggers();
        observers.resize( num_triggers );
        disposed_observers.resize( num_triggers );
    }

    template<class Observer, class Entity, class Trigger, class TriggerFactory>
    BroadcasterImpl<Observer, Entity, Trigger, TriggerFactory>::~BroadcasterImpl()
    {
        DisposeOfUnregisteredObservers();

        for( auto &observer_list : observers )
        {
            LOG_DEBUG_F( "Deleting %d observers.\n", observer_list.size() );

            for( auto &observer : observer_list )
            {
                observer->Release();
            }
        }
    }

    template<class Observer, class Entity, class Trigger, class TriggerFactory>
    void BroadcasterImpl<Observer, Entity, Trigger, TriggerFactory>::RegisterObserver( Observer* pObserver, const Trigger& trigger )
    {
        std::vector<Observer*>& event_observer_list = observers[ trigger.GetIndex() ];

        if( std::find( event_observer_list.begin(), event_observer_list.end(), pObserver ) != event_observer_list.end() )
        {
            std::stringstream ss;
            ss << "Trying to register an observer (" << typeid(*pObserver).name() << ") more than once to event " << trigger.ToString();
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        LOG_DEBUG_F( "Observer is registering for event %s.\n", trigger.c_str() );
        event_observer_list.push_back( pObserver );
        pObserver->AddRef();
    }

    template<class Observer, class Entity, class Trigger, class TriggerFactory>
    void BroadcasterImpl<Observer, Entity, Trigger, TriggerFactory>::UnregisterObserver( Observer* pObserver, const Trigger& trigger )
    {
        LOG_DEBUG( "[UnregisterObserver] Putting observer into the disposed observers list .\n" );
        disposed_observers[ trigger.GetIndex() ].push_back( pObserver );
    }

    template<class Observer, class Entity, class Trigger, class TriggerFactory>
    void BroadcasterImpl<Observer, Entity, Trigger, TriggerFactory>::TriggerObservers( Entity* pEntity, const Trigger& trigger )
    {
        if( trigger.IsUninitialized() )
        {
            return;
        }
        ++num_triggered_events;

        std::vector<Observer*>& observer_list = observers[ trigger.GetIndex() ];
        std::vector<Observer*>& disposed_list = disposed_observers[ trigger.GetIndex() ];

        if( observer_list.size() > 0 )
        {
            ++num_observed_events;
        }

        LOG_DEBUG_F( "We have %d observers of event %s.\n", observer_list.size(), trigger.c_str() );
        for( auto observer : observer_list )
        {
            // ---------------------------------------------------------------------
            // --- Make sure the observer has not been requested to be unregistered
            // --- from being notified of events.
            // ---------------------------------------------------------------------
            bool notify = true;
            if( disposed_list.size() > 0 )
            {
                // finding the observer will make notify FALSE which means we don't notify them of the event
                notify = std::find( disposed_list.begin(), disposed_list.end(), observer ) == disposed_list.end();
            }

            if( notify )
            {
                observer->notifyOnEvent( pEntity, trigger );
            }
        }
    }

    template<class Observer, class Entity, class Trigger, class TriggerFactory>
    uint64_t BroadcasterImpl<Observer, Entity, Trigger, TriggerFactory>::GetNumTriggeredEvents()
    {
        // ------------------------------------------------------------------------------------------
        // --- By clearing the value after getting it, we are attempting to ensure we include
        // --- all of the events since we last asked for it.
        // ------------------------------------------------------------------------------------------
        uint64_t ret = num_triggered_events;
        num_triggered_events = 0;
        return ret;
    }

    template<class Observer, class Entity, class Trigger, class TriggerFactory>
    uint64_t BroadcasterImpl<Observer, Entity, Trigger, TriggerFactory>::GetNumObservedEvents()
    {
        // ------------------------------------------------------------------------------------------
        // --- By clearing the value after getting it, we are attempting to ensure we include
        // --- all of the events since we last asked for it.
        // ------------------------------------------------------------------------------------------
        uint64_t ret = num_observed_events;
        num_observed_events = 0;
        return ret;
    }

    template<class Observer, class Entity, class Trigger, class TriggerFactory>
    void BroadcasterImpl<Observer, Entity, Trigger, TriggerFactory>::DisposeOfUnregisteredObservers()
    {
        if( disposed_observers.size() > 0 )
        {
            LOG_DEBUG_F( "We have %d disposed_observers to clean up.\n", disposed_observers.size() );
        }

        for( int event_index = 0; event_index < disposed_observers.size(); ++event_index )
        {
            std::vector<Observer*>& disposed_list = disposed_observers[ event_index ];
            std::vector<Observer*>& current_list = observers[ event_index ];

            for( auto observer : disposed_list )
            {
                for( int i = 0; i < current_list.size(); ++i )
                {
                    if( current_list[ i ] == observer )
                    {
                        current_list[ i ] = current_list.back();
                        current_list.pop_back();
                        LOG_DEBUG_F( "[UnregisterObserver] Removed observer from list: now %d observers of event %s.\n",
                                    current_list.size(),
                                    TriggerFactory::GetInstance()->GetEventTriggerName( event_index ).c_str()
                        );
                        break;
                    }
                }
            }
            disposed_list.clear();
        }
    }
}
