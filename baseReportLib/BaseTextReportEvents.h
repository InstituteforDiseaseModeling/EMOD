/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>

#include "BaseTextReport.h"
#include "BroadcasterObserver.h"
#include "EventTrigger.h"

namespace Kernel 
{
    struct INodeEventContext;

    template<class Broadcaster, class Observer, class Entity, class Trigger>
    class BaseTextReportEventsTemplate : public BaseTextReport, public Observer
    {
    public:
        BaseTextReportEventsTemplate( const std::string& rReportName );
        virtual ~BaseTextReportEventsTemplate();

        // ------------
        // --- IReport
        // ------------
        virtual void Reduce() override;

        // --------------
        // --- ISupports
        // --------------
        virtual Kernel::QueryResult QueryInterface( Kernel::iid_t iid, void **ppvObject ) override { return Kernel::e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()

    protected:
        void UpdateRegistration( Broadcaster* broadcaster, bool registering );
        void UnregisterAllBroadcasters();

        // this is not private so that subclasses can use initConfig() to initialize it.
        std::vector< Trigger > eventTriggerList;

        std::vector< Broadcaster* > broadcaster_list;
        bool is_registered;
    };

    // BaseTextReportEvents is an abstract base class that manages the handling of events
    // and the output of a text file.  The class derived from this class just needs to
    // worry about defining the header of the file, setting the events to listen for,
    // and adding the data.
    class BaseTextReportEvents : public BaseTextReportEventsTemplate<IIndividualEventBroadcaster,
                                                                     IIndividualEventObserver,
                                                                     IIndividualHumanEventContext,
                                                                     EventTrigger>
    {
    public:
        BaseTextReportEvents( const std::string& rReportName );
        virtual ~BaseTextReportEvents();

        // ------------
        // --- IReport
        // ------------
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
    };
}
