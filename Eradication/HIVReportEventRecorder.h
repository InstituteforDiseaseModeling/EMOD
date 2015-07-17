/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <vector>

#include "ReportEventRecorder.h"

namespace Kernel
{
    struct ISimulation;

    class HIVReportEventRecorder : public ReportEventRecorder
    {
        GET_SCHEMA_STATIC_WRAPPER(HIVReportEventRecorder)

    public:
        static IReport* CreateReport();

    protected:
        // allowed "sim" to default to nullptr so that this constructor can be used in generating the schema
        HIVReportEventRecorder();
        virtual ~HIVReportEventRecorder();

        // -----------------------------
        // --- BaseTextReportEvents
        // -----------------------------
        virtual std::string GetHeader() const ;

        // -----------------------
        // --- ReportEventRecorder
        // -----------------------
        virtual std::string GetOtherData( IIndividualHumanEventContext *context, const std::string& StateChange );
    };
}
