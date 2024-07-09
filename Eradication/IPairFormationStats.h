
#pragma once
#include "IdmApi.h"
#include "SimulationEnums.h"
#include "ISerializable.h"
#include <map>
#include <vector>

using namespace std;

namespace Kernel 
{
    struct IPairFormationParameters;

    struct IDMAPI IPairFormationStats  : ISerializable
    {
        virtual void ResetEligible() = 0;
        virtual void UpdateEligible(float age_in_days, int sex, RiskGroup::Enum risk_group, int delta) = 0;
        virtual const map<int, vector<int>>& GetEligible(RiskGroup::Enum risk_group) = 0;
        virtual void SetParameters( const IPairFormationParameters* params ) = 0;

        virtual ~IPairFormationStats() {}
    };
}