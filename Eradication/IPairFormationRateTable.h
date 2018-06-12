/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "IdmApi.h"
#include "SimulationEnums.h"
#include "ISerializable.h"

namespace Kernel 
{
    struct IPairFormationParameters;

    struct IDMAPI IPairFormationRateTable : ISerializable
    {
        virtual float GetRateForAgeAndSexAndRiskGroup(float age, int sex, RiskGroup::Enum risk_group) const = 0;
        virtual void SetRateForBinAndSexAndRiskGroup(int bin_index, int sex, RiskGroup::Enum risk_group, float value) = 0;
        virtual void SetParameters( const IPairFormationParameters* params ) = 0;
        virtual void DumpRates() = 0;
        virtual ~IPairFormationRateTable() {}
    };
}