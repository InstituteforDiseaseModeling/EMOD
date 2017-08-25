/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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

    protected:
        virtual std::string GetTimeHeader() const;
        virtual float GetTime( const IdmDateTime& rDateTime ) const;
    };
}
