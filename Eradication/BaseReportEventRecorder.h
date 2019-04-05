/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>

#include "BaseTextReportEvents.h"
#include "BroadcasterObserver.h"

namespace Kernel
{
    struct IdmDateTime;

    template<class Broadcaster, class Observer, class Entity, class Trigger, class Factory>
    class BaseReportEventRecorder : public BaseTextReportEventsTemplate<Broadcaster, Observer, Entity, Trigger>
    {
    public:
        static std::string GetEnableParameterName();

    protected:
        static const std::string ENABLE_PARAMETER_NAME;
        static const std::string EVENTS_LIST_NAME;
        static const std::string EVENTS_LIST_DESC;
        static const std::string IGNORE_EVENTS_LIST_NAME;
        static const std::string IGNORE_EVENTS_LIST_DESC;

        BaseReportEventRecorder( const std::string& rReportName );
        virtual ~BaseReportEventRecorder();

        // -----------------------------
        // --- BaseTextReportEvents
        // -----------------------------
        virtual bool Configure( const Configuration* inputJson ) override;
        virtual std::string GetHeader() const;

        // IObserver
        bool notifyOnEvent( Entity *pEntity, const Trigger& trigger );

    protected:
        virtual void ConfigureOther( const Configuration* inputJson );
        virtual std::string GetOtherData( Entity *pEntity, const Trigger& trigger );
        virtual std::string GetTimeHeader() const;

        virtual float GetTime( Entity* pEntity ) const = 0;

        bool ignore_events_in_list;

        std::string m_EnableParameterName;
        std::string m_EventsListName;
        std::string m_EventsListDesc;
        std::string m_IgnoreEventsListName;
        std::string m_IgnoreEventsListDesc;
    };
}
