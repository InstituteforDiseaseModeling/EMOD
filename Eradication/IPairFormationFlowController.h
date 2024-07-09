
#pragma once
#include "IdmApi.h"
#include "IdmDateTime.h"
#include "ISerializable.h"

namespace Kernel
{
    struct IDMAPI IPairFormationFlowController : ISerializable
    {
        virtual void UpdateEntryRates( const IdmDateTime& rCurrentTime, float dt ) = 0;
        virtual std::map<int, std::vector<float>>& GetDesiredFlow() = 0;
        virtual ~IPairFormationFlowController() {}
    };
}