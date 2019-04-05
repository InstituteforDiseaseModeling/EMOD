/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <string>
#include "TyphoidReportEventRecorder.h"
#include "IdmDateTime.h"
#include "NodeEventContext.h"

SETUP_LOGGING( "TyphoidReportEventRecorder" )

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL( TyphoidReportEventRecorder, TyphoidReportEventRecorder )

    IReport* TyphoidReportEventRecorder::CreateReport()
    {
        return new TyphoidReportEventRecorder();
    }

    TyphoidReportEventRecorder::TyphoidReportEventRecorder()
        : ReportEventRecorder()
    {
    }

    TyphoidReportEventRecorder::~TyphoidReportEventRecorder()
    {
    }

    std::string TyphoidReportEventRecorder::GetTimeHeader() const
    {
        return "Year";
    }

    float TyphoidReportEventRecorder::GetTime( IIndividualHumanEventContext* pEntity ) const
    {
        return pEntity->GetNodeEventContext()->GetTime().Year();
    }
}
