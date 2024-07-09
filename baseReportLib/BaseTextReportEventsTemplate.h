
#pragma once

#include <vector>

#include "BaseTextReportEvents.h"

namespace Kernel 
{
    // ------------------------------------------------------------------------
    // --- BaseTextReportEventsTemplate
    // ------------------------------------------------------------------------

    template<class Broadcaster, class Observer, class Entity, class Trigger>
    BaseTextReportEventsTemplate<Broadcaster, Observer, Entity, Trigger>::BaseTextReportEventsTemplate( const std::string& rReportName )
        : BaseTextReport( rReportName )
        , eventTriggerList()
        , broadcaster_list()
        , is_registered( false )
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger>
    BaseTextReportEventsTemplate<Broadcaster, Observer, Entity, Trigger>::BaseTextReportEventsTemplate( const BaseTextReportEventsTemplate& rThat )
        : BaseTextReport( rThat )
        , eventTriggerList( rThat.eventTriggerList )
        , broadcaster_list()
        , is_registered( rThat.is_registered )
    {
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger>
    BaseTextReportEventsTemplate<Broadcaster, Observer, Entity, Trigger>::~BaseTextReportEventsTemplate()
    {
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger>
    void BaseTextReportEventsTemplate<Broadcaster, Observer, Entity, Trigger>::Reduce()
    {
        BaseTextReport::Reduce();
        if( is_registered )
        {
            UnregisterAllBroadcasters();
        }
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger>
    void BaseTextReportEventsTemplate<Broadcaster, Observer, Entity, Trigger>::UpdateRegistration( Broadcaster* broadcaster, bool registering )
    {
        for( auto trigger : eventTriggerList )
        {
            if( registering )
            {
                LOG_DEBUG_F( "BaseTextReportEventsTemplate is registering to listen to event %s\n", trigger.c_str() );
                broadcaster->RegisterObserver( this, trigger );
            }
            else
            {
                broadcaster->UnregisterObserver( this, trigger );
            }
        }
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger>
    void BaseTextReportEventsTemplate<Broadcaster, Observer, Entity, Trigger>::UnregisterAllBroadcasters()
    {
        for( auto broadcaster : broadcaster_list )
        {
            UpdateRegistration( broadcaster, false );
            broadcaster->Release();
        }
        broadcaster_list.clear();
    }

}
