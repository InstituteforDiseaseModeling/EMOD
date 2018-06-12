/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "IdmApi.h"
#include "IdmDateTime.h"
#include "ISerializable.h"

namespace Kernel
{
    struct IDMAPI IPairFormationFlowController : ISerializable
    {
        virtual void UpdateEntryRates( const IdmDateTime& rCurrentTime, float dt ) = 0;
        virtual ~IPairFormationFlowController() {}
    };
}