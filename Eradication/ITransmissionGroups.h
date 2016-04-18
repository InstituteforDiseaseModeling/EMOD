/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>

#include "Properties.h"
#include "StrainIdentity.h"
#include "IInfectable.h"
#include "Types.h"      // for act_prob_vec_t

namespace Kernel
{
    typedef list<string>             PropertyValueList_t;   // Use a list to preserve the order
    typedef vector<float>            MatrixRow_t;
    typedef vector<MatrixRow_t>      ScalingMatrix_t;
    typedef map<const string, float> RouteToContagionDecayMap_t;
    typedef set<StrainIdentity*>     StrainIdentitySet_t;   // Order isn't important here
    typedef vector<string>           RouteList_t;    

    // Forward declare this so that it is available but opaque to the consumers.
    class TransmissionGroupMembership_t;

    struct ITransmissionGroups
    {
        virtual void AddProperty(const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix, const string& route) = 0;
        virtual void Build(const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, int numberOfAntigens, int numberOfSubstrains) = 0;
        virtual void GetGroupMembershipForProperties(const RouteList_t& route, const tProperties* properties, TransmissionGroupMembership_t* membershipOut) const = 0;
        virtual void UpdatePopulationSize(const TransmissionGroupMembership_t* transmissionGroupMembership, float size_changes, float mc_weight) = 0;
        virtual float GetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership) = 0;
        virtual void DepositContagion(const StrainIdentity* strain, float amount, const TransmissionGroupMembership_t* transmissionGroupMembership) = 0;
        virtual void ExposeToContagion(IInfectable* candidate, const TransmissionGroupMembership_t* transmissionGroupMembership, float deltaTee) const = 0;
        virtual void CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership) = 0;
        virtual void IncrementWeightedPopulation(const TransmissionGroupMembership_t* transmissionGroupMembership, float weight) = 0;
        virtual void EndUpdate(float infectivityCorrection) = 0;

        virtual act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership) = 0;
    };
}
