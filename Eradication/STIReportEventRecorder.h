
#pragma once

#include "ReportEventRecorder.h"

namespace Kernel
{
    class STIReportEventRecorder : public ReportEventRecorder
    {
        GET_SCHEMA_STATIC_WRAPPER( STIReportEventRecorder )
    public:
        static IReport* CreateReport();

    protected:
        STIReportEventRecorder();
        virtual ~STIReportEventRecorder();

        virtual std::string GetTimeHeader() const override;
        virtual std::string GetTime( IIndividualHumanEventContext* pEntity ) const override;
    };
}
