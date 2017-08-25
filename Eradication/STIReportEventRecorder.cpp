/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <string>
#include "STIReportEventRecorder.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "STIReportEventRecorder" )

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL( STIReportEventRecorder, STIReportEventRecorder )

    IReport* STIReportEventRecorder::CreateReport()
    {
        return new STIReportEventRecorder();
    }

    STIReportEventRecorder::STIReportEventRecorder()
        : ReportEventRecorder()
    {
    }

    STIReportEventRecorder::~STIReportEventRecorder()
    {
    }

    std::string STIReportEventRecorder::GetTimeHeader() const
    {
        return "Year";
    }

    float STIReportEventRecorder::GetTime( const IdmDateTime& rDateTime ) const
    {
        return rDateTime.Year();
    }
}
