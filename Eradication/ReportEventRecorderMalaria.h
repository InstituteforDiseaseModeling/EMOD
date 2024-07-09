
#pragma once

#include <vector>

#include "ReportEventRecorder.h"

namespace Kernel
{
    struct ISimulation;

    class ReportEventRecorderMalaria : public ReportEventRecorder
    {
    public:
        static IReport* CreateReport();

    protected:
        // allowed "sim" to default to nullptr so that this constructor can be used in generating the schema
        ReportEventRecorderMalaria();
        virtual ~ReportEventRecorderMalaria();

        // -----------------------------
        // --- BaseTextReportEvents
        // -----------------------------
        virtual std::string GetHeader() const override;

        // -----------------------
        // --- ReportEventRecorder
        // -----------------------
        virtual std::string GetOtherData( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;
    };
}
