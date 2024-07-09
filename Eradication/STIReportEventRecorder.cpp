
#include "stdafx.h"

#include <string>
#include "STIReportEventRecorder.h"
#include "IdmDateTime.h"
#include "NodeEventContext.h"
#include "SimulationEventContext.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "STIReportEventRecorder" )

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(STIReportEventRecorder,STIReportEventRecorder)

    IReport* STIReportEventRecorder::CreateReport()
    {
        return new STIReportEventRecorder();
    }

    STIReportEventRecorder::STIReportEventRecorder()
        : ReportEventRecorder(true)
    {
    }

    STIReportEventRecorder::~STIReportEventRecorder()
    {
    }

    std::string STIReportEventRecorder::GetTimeHeader() const
    {
        std::stringstream ss;
        ss << ReportEventRecorder::GetTimeHeader() << ",Year";
        return ss.str();
    }

    std::string STIReportEventRecorder::GetTime( IIndividualHumanEventContext* pEntity ) const
    {
        std::stringstream ss;
        ss << ReportEventRecorder::GetTime( pEntity ) << "," << pEntity->GetNodeEventContext()->GetTime().Year();
        return ss.str();
    }
}
