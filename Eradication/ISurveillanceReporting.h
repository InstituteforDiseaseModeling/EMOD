
#pragma once

#include "ISupports.h"

namespace Kernel
{
    class ReportStatsByIP;

    // When the SurveillanceEventCoordinator like object determines it needs to respond
    // to what it is observing, we can collect data about that decision.  Some of this
    // data is about the environment that is visible to the coordinator - i.e. the number
    // of individuals in the nodes that the coordinator can see and that meet any restrictions
    // being used by the coordinator.  The purpose of the interface is to provide access to the
    // information about the decision to respond and the current state.
    struct ISurveillanceReporting : ISupports
    {
        virtual void CollectStats( ReportStatsByIP& rStats ) = 0;
        virtual uint32_t GetNumCounted() const = 0;
        virtual float GetCurrentActionThreshold() const = 0;
    };
}
