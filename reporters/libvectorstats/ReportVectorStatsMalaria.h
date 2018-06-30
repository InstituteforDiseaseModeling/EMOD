/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ReportVectorStats.h"

namespace Kernel
{
    class ReportVectorStatsMalaria : public ReportVectorStats
    {
    public:
        ReportVectorStatsMalaria();
        virtual ~ReportVectorStatsMalaria();

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual std::string GetHeader() const override;

    protected:
        virtual void ResetOtherCounters() override;
        virtual void CollectOtherData( IVectorPopulationReporting* pIVPR ) override;
        virtual void WriteOtherData() override;

    private:
        std::vector<ReportUtilitiesMalaria::GenomeMarkerColumn> genome_marker_columns;
    };
}
