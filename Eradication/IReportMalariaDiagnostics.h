
#pragma once

#include <vector>
#include "ISupports.h"

namespace Kernel
{
    struct IReportMalariaDiagnostics : ISupports
    {
        virtual void SetDetectionThresholds( const std::vector<float>& rDetectionThresholds ) = 0;
    };
}
