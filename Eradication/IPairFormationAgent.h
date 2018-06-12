/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IdmApi.h"
#include "IdmDateTime.h"
#include "IIndividualHumanSTI.h"
#include "Configure.h"
#include "ISerializable.h"
#include <map>
#include <vector>

using namespace std;

namespace Kernel
{
    struct IDMAPI IPairFormationAgent : virtual ISerializable, JsonConfigurable
    {
        virtual void AddIndividual(IIndividualHumanSTI*) = 0;
        virtual void RemoveIndividual(IIndividualHumanSTI*) = 0;
        virtual void Update( const IdmDateTime& rCurrentTime, float dt ) = 0;
        virtual const map<int, vector<float>>& GetAgeBins() = 0;
        virtual const map<int, vector<float>>& GetDesiredFlow() = 0;
        virtual const map<int, vector<int>>& GetQueueLengthsBefore() = 0;
        virtual const map<int, vector<int>>& GetQueueLengthsAfter() = 0;
        virtual void Print(const char *) const = 0;
        virtual ~IPairFormationAgent() {}
    };
}