/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>

#include "BaseReportEventRecorder.h"

namespace Kernel
{
    class EventTriggerFactory;

    class ReportEventRecorder : public BaseReportEventRecorder< IIndividualEventBroadcaster,
                                                                IIndividualEventObserver,
                                                                IIndividualHumanEventContext,
                                                                EventTrigger,
                                                                EventTriggerFactory >
    {
    public:
        GET_SCHEMA_STATIC_WRAPPER( ReportEventRecorder )

    public:
        static IReport* CreateReport();

    public:
        ReportEventRecorder();
        virtual ~ReportEventRecorder();

        // ------------
        // --- IReport
        // ------------
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;

        virtual std::string GetHeader() const override;

    protected:
        virtual void ConfigureOther( const Configuration* inputJson ) override;
        virtual std::string GetOtherData( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;
        virtual float GetTime( IIndividualHumanEventContext* pEntity ) const override;

        jsonConfigurable::tDynamicStringSet properties_to_report;
    };
}
