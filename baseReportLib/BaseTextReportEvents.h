/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>

#include "BaseTextReport.h"
#include "Interventions.h"
#include "EventTrigger.h"

namespace Kernel 
{
    struct INodeEventContext;

    // BaseTextReportEvents is an abstract base class that manages the handling of events
    // and the output of a text file.  The class derived from this class just needs to
    // worry about defining the header of the file, setting the events to listen for,
    // and adding the data.
    class BaseTextReportEvents : public BaseTextReport, public IIndividualEventObserver
    {
    public:
        BaseTextReportEvents( const std::string& rReportName );
        virtual ~BaseTextReportEvents();

        // ------------
        // --- IReport
        // ------------
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList ) override;
        virtual void Reduce() override;

        // --------------
        // --- ISupports
        // --------------
        virtual Kernel::QueryResult QueryInterface(Kernel::iid_t iid, void **ppvObject) override { return Kernel::e_NOINTERFACE; }
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    protected:
        void UpdateRegistration( INodeTriggeredInterventionConsumer* pNTIC, bool registering );
        void UnregisterAllNodes();

        // this is not private so that subclasses can use initConfig() to initialize it.
        std::vector< EventTrigger > eventTriggerList ;

    private:
        std::vector< INodeTriggeredInterventionConsumer* > ntic_list ;
        bool is_registered ;
    };
}
