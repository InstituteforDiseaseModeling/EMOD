
#pragma once

#include "BaseReportEventRecorder.h"
#include "Log.h"

namespace Kernel
{
    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    std::string BaseReportEventRecorder<Broadcaster, Observer, Entity, Trigger, Factory>::GetEnableParameterName()
    {
        return ENABLE_PARAMETER_NAME;
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    BaseReportEventRecorder<Broadcaster,Observer,Entity,Trigger,Factory>::BaseReportEventRecorder( const std::string& rReportName )
        : BaseTextReportEventsTemplate<Broadcaster, Observer, Entity, Trigger>( rReportName )
        , ignore_events_in_list( false )
        , m_EnableParameterName( ENABLE_PARAMETER_NAME )
        , m_EventsListName( EVENTS_LIST_NAME )
        , m_EventsListDesc( EVENTS_LIST_DESC )
        , m_IgnoreEventsListName( IGNORE_EVENTS_LIST_NAME )
        , m_IgnoreEventsListDesc( IGNORE_EVENTS_LIST_DESC )
    {
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    BaseReportEventRecorder<Broadcaster, Observer, Entity, Trigger, Factory>::~BaseReportEventRecorder()
    {
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    bool BaseReportEventRecorder<Broadcaster, Observer, Entity, Trigger, Factory>::Configure( const Configuration * inputJson )
    {
        std::vector<Trigger> tmp_event_trigger_list;

        JsonConfigurable::initConfigTypeMap( m_EventsListName.c_str(),       &tmp_event_trigger_list, m_EventsListDesc.c_str(), m_EnableParameterName.c_str() );
        JsonConfigurable::initConfigTypeMap( m_IgnoreEventsListName.c_str(), &ignore_events_in_list,  m_IgnoreEventsListDesc.c_str(), false, m_EnableParameterName.c_str() );

        ConfigureOther( inputJson );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            CheckOther( inputJson );
            if( !ignore_events_in_list && tmp_event_trigger_list.empty() )
            {
                std::stringstream ss;
                ss << "No data will be recorded.  The " << m_EventsListName << " list is empty and " << m_IgnoreEventsListName << " is false.\n";
                LOG_WARN( ss.str().c_str() );
            }
            else
            {
                // This logic goes through all possible events.  It checks to see if that event
                // is in the listen-to-these event_list provided by the user. But that list can be a 
                // whitelist or blacklist. If using whitelist AND event-requested is in master THEN listen.
                // else if using blacklist AND if event-(de)requested is not in master THEN listen.

                std::vector<Trigger> all_trigger_list = Factory::GetInstance()->GetAllEventTriggers();
                for( auto trigger : all_trigger_list )
                {
                    bool in_event_list = std::find( tmp_event_trigger_list.begin(),
                                                    tmp_event_trigger_list.end(), trigger ) != tmp_event_trigger_list.end();

                    if( ignore_events_in_list != in_event_list )
                    {
                        // list of events to listen for
                        BaseTextReportEventsTemplate<Broadcaster, Observer, Entity, Trigger>::eventTriggerList.push_back( trigger );
                    }
                }
            }
        }

        return ret;
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    void BaseReportEventRecorder<Broadcaster, Observer, Entity, Trigger, Factory>::ConfigureOther( const Configuration* inputJson )
    {
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    void BaseReportEventRecorder<Broadcaster, Observer, Entity, Trigger, Factory>::CheckOther( const Configuration* inputJson )
    {
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    std::string BaseReportEventRecorder<Broadcaster, Observer, Entity, Trigger, Factory>::GetHeader() const
    {
        return GetTimeHeader();
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    bool BaseReportEventRecorder<Broadcaster, Observer, Entity, Trigger, Factory>::notifyOnEvent( Entity *pEntity, const Trigger& trigger )
    {
        GetOutputStream() << GetTime( pEntity );

        GetOutputStream() << GetOtherData( pEntity, trigger );

        GetOutputStream() << std::endl;

        return true;
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    std::string BaseReportEventRecorder<Broadcaster, Observer, Entity, Trigger, Factory>::GetTimeHeader() const
    {
        return "Time";
    }

    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    std::string BaseReportEventRecorder<Broadcaster, Observer, Entity, Trigger, Factory>::GetOtherData( Entity *pEntity, const Trigger& trigger )
    {
        return "";
    }
}
