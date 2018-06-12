/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ReportVector.h"

namespace Kernel {

    class ReportMalaria : public ReportVector
    {
    public:
        ReportMalaria();
        virtual ~ReportMalaria() {};

        static IReport* ReportMalaria::CreateReport() { return new ReportMalaria(); }

        virtual void LogNodeData( Kernel::INodeContext * pNC );

    protected:
        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );
        virtual void postProcessAccumulatedData();
    };
}
