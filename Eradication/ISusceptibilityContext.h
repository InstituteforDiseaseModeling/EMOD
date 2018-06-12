/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "ISerializable.h"

namespace Kernel
{
    struct ISusceptibilityContext : ISerializable
    {
        virtual float getAge() const = 0;
        virtual float getModAcquire() const = 0;
        virtual float getModTransmit() const = 0;
        virtual float getModMortality() const = 0;
        virtual float getImmuneFailage() const = 0;
        virtual void  updateModAcquire(float updateVal) = 0;
        virtual void  updateModTransmit(float updateVal) = 0;
        virtual void  updateModMortality(float updateVal) = 0;
        virtual void  setImmuneFailage(float newFailage) = 0;
        virtual void  InitNewInfection() = 0;
        virtual bool  IsImmune() const = 0;

        virtual ~ISusceptibilityContext() {}
    };
}