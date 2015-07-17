/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <vector>

#include "BaseTextReportEvents.h"
#include "Interventions.h"

namespace Kernel
{
    class ReportEventRecorder : public BaseTextReportEvents
    {
        GET_SCHEMA_STATIC_WRAPPER(ReportEventRecorder)
    public:
        static IReport* CreateReport();

    protected:
        ReportEventRecorder();
        virtual ~ReportEventRecorder();

        // -----------------------------
        // --- BaseTextReportEvents
        // -----------------------------
        virtual bool Configure( const Configuration* inputJson );
        virtual std::string GetHeader() const ;
        virtual bool notifyOnEvent(IIndividualHumanEventContext *context, const std::string& StateChange);

    protected:
        virtual std::string GetOtherData( IIndividualHumanEventContext *context, const std::string& StateChange );

        JsonConfigurable::tDynamicStringSet properties_to_report;

    private:
        bool ignore_events_in_list ;
    };
}
