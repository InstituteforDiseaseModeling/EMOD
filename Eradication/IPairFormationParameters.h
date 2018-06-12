/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IdmApi.h"
#include "SimulationEnums.h"
#include "IRelationship.h"
#include "IdmDateTime.h"
#include <map>
#include <vector>
#include "ISerializable.h"

using namespace std;

namespace Kernel 
{

    struct IDMAPI IPairFormationParameters : virtual ISerializable
    {
        virtual RelationshipType::Enum GetRelationshipType() const = 0;

        virtual int GetMaleAgeBinCount() const = 0;
        virtual float GetInitialMaleAge() const = 0;
        virtual float GetMaleAgeIncrement() const = 0;

        virtual int GetFemaleAgeBinCount() const = 0;
        virtual float GetInitialFemaleAge() const = 0;
        virtual float GetFemaleAgeIncrement() const = 0;

        virtual float GetRateRatio(Gender::Enum gender) const = 0;

        virtual const map<int, vector<float>>& GetAgeBins() const = 0;
        virtual const int BinIndexForAgeAndSex( float age_in_days, int sex ) const = 0;

        virtual const vector<vector<float>>& JointProbabilityTable() const = 0;        // male age bins X female age bins
        virtual const vector<vector<float>>& CumulativeJointProbabilityTable() const = 0;      // male age bins X female age bins

        virtual const map<int, vector<float>>& MarginalValues() const = 0;

        virtual const vector<vector<float>>& AperpPseudoInverse() const = 0;              // female age bins X female age bins
        virtual const vector<vector<float>>& OrthogonalBasisForATranspose() const = 0;    // (male + female) age bins X female age bins

        virtual const vector<float>& SingularValues() const = 0;                          // female age bins count

        virtual float FormationRate( const IdmDateTime& rCurrentTime, float dt ) const = 0;
        virtual float UpdatePeriod() const = 0;

        virtual ~IPairFormationParameters() {}
    };
}