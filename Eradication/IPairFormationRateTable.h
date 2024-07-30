
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