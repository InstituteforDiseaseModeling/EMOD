/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "StrainAwareTransmissionGroups.h"

namespace Kernel
{
    class VectorTransmissionGroups : protected StrainAwareTransmissionGroups
    {
    public:
        VectorTransmissionGroups();

    protected:

    private:

        // ITransmissionGroups implementation
        // Same as MultiRouteTransmissionGroups
        // virtual void AddProperty(const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix, const string& route);
        // Same as MultiRouteTransmissionGroups
        // virtual void Build(const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, const StrainIdentitySet_t& strains, int numberOfSubstrains);
        // Same as MultiRouteTransmissionGroups
        // virtual const TransmissinGroupMembership_t* GetGroupMembershipForProperties(const tProperties* properties) const;
        // Same as MultiRouteTransmissionGroups
        // virtual void DepositContagion(const IStrainIdentity& strain, float amount, const TransmissionGroupMembership_t* transmissionGroupMembership);
        virtual void ExposeToContagion(IInfectable* candidate, const TransmissionGroupMembership_t* transmissionGroupMembership, float deltaTee) const;
    };
}