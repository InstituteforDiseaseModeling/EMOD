/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include "ISerializable.h"
#include "Configuration.h"
#include "IdmDateTime.h"
#include "IdmApi.h"

namespace Kernel
{
    struct INodeContext;

    struct IDMAPI ISimulation : ISerializable
    {
        virtual void Initialize(const ::Configuration *config) = 0;
        virtual bool Populate() = 0;
        virtual void Update(float time_step) = 0;

        virtual int  GetSimulationTimestep() const = 0;
        virtual IdmDateTime GetSimulationTime() const = 0;

        typedef std::function<void(INodeContext*)> callback_t;
        virtual void RegisterNewNodeObserver(void* id, Kernel::ISimulation::callback_t observer) = 0;
        virtual void UnregisterNewNodeObserver(void* id) = 0;

        virtual void WriteReportsData() = 0;
        virtual ~ISimulation() {}
    };
}
