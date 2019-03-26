/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ReportEventRecorder.h"

namespace Kernel
{
    class TyphoidReportEventRecorder : public ReportEventRecorder
    {
        GET_SCHEMA_STATIC_WRAPPER( TyphoidReportEventRecorder )
    public:
        static IReport* CreateReport();

    protected:
        TyphoidReportEventRecorder();
        virtual ~TyphoidReportEventRecorder();

    protected:
        virtual std::string GetTimeHeader() const;
        virtual float GetTime( IIndividualHumanEventContext* pEntity ) const override;
    };
}
