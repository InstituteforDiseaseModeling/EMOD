
#pragma once

#include <vector>

#include "STIReportEventRecorder.h"
#include "Properties.h"

namespace Kernel
{
    struct ISimulation;

    class HIVReportEventRecorder : public STIReportEventRecorder
    {
    public:
        static IReport* CreateReport();

    protected:
        // allowed "sim" to default to nullptr so that this constructor can be used in generating the schema
        HIVReportEventRecorder();
        virtual ~HIVReportEventRecorder();

        // -----------------------------
        // --- BaseTextReportEvents
        // -----------------------------
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;

        // -----------------------
        // --- ReportEventRecorder
        // -----------------------
        virtual std::string GetOtherData( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;

        IPKey m_InterventionStatusKey;
    };
}
