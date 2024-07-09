
#pragma once

#include "ReportFilter.h"


namespace Kernel
{
    struct IRelationship;

    class ReportFilterRelationship : public ReportFilter
    {
    public:
        ReportFilterRelationship( const char* pReportEnableParameterName,
                                  const char* pParamNamePrefix );
        ~ReportFilterRelationship();

        bool IsValidRelationship( IRelationship* pRel ) const;

    protected:
    };
}
