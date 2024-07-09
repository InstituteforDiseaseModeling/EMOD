
#pragma once

#include <vector>

#include "ReportEventRecorder.h"

namespace Kernel
{
    struct ISimulation;

    class ReportEventRecorderMalariaCoTran : public ReportEventRecorder
    {
    public:
        static IReport* CreateReport();

    protected:
        // allowed "sim" to default to nullptr so that this constructor can be used in generating the schema
        ReportEventRecorderMalariaCoTran();
        virtual ~ReportEventRecorderMalariaCoTran();

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
