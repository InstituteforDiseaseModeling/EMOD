/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>

#include "PropertiesString.h"
#include "IInfectable.h"
#include "Types.h"      // for act_prob_vec_t
#include "Properties.h"

namespace Kernel
{
    class StrainIdentity;

    typedef std::list<std::string>             PropertyValueList_t;   // Use a list to preserve the order
    typedef std::vector<float>                 MatrixRow_t;
    typedef std::vector<MatrixRow_t>           ScalingMatrix_t;
    typedef std::set<StrainIdentity*>          StrainIdentitySet_t;   // Order isn't important here
    typedef std::vector<std::string>           RouteList_t;

    // Forward declare this so that it is available but opaque to the consumers.
    struct TransmissionGroupMembership_t;

    struct ITransmissionGroups
    {
        virtual void AddProperty(const std::string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix) = 0;
        virtual void Build(float contagionDecayRate, int numberOfAntigens, int numberOfSubstrains) = 0;
        virtual void GetGroupMembershipForProperties(const tProperties& properties, TransmissionGroupMembership_t& membershipOut) const = 0;
        virtual void UpdatePopulationSize(const TransmissionGroupMembership_t& transmissionGroupMembership, float size_changes, float mc_weight) = 0;
        virtual void DepositContagion(const IStrainIdentity& strain, float amount, TransmissionGroupMembership_t transmissionGroupMembership) = 0;
        virtual void ExposeToContagion(IInfectable* candidate, TransmissionGroupMembership_t transmissionGroupMembership, float deltaTee, TransmissionRoute::Enum tx_route = TransmissionRoute::TRANSMISSIONROUTE_CONTACT) const = 0;
        virtual void CorrectInfectivityByGroup(float infectivityCorrection, TransmissionGroupMembership_t transmissionGroupMembership) = 0;
        virtual void EndUpdate(float infectivityMultiplier = 1.0f, float infectivityAddition = 0.0f ) = 0;
        virtual float GetContagionByProperty( const IPKeyValue& property_value ) = 0;

        virtual void UseTotalPopulationForNormalization( void ) = 0;
        virtual void UseGroupPopulationForNormalization( void ) = 0;

        virtual void SetTag( const std::string& tag ) = 0;
        virtual const std::string& GetTag( void ) const = 0;

        virtual float GetTotalContagion( void ) = 0;                                            // Return total contagion.
        virtual float GetTotalContagionForGroup( TransmissionGroupMembership_t txGroups) = 0;   // Return total contagion for given membership.
// NOTYET        virtual float GetTotalContagionForProperties( const IPKeyValueContainer& property_value ) = 0;                              // Return total contagion for given properties (maps to membership).

        virtual act_prob_vec_t DiscreteGetTotalContagion( void ) = 0;
    };
}
