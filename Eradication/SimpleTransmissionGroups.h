/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "TransmissionGroupsBase.h"
#include "IContagionPopulation.h"

namespace Kernel
{
    /**************************************************************************
    **
    ** SimpleTransmissionGroups support heterogeneous intra-node transmission with:
    **    - a single route of transmission
    **    - a single major strain (antigen id)
    **    - a single substrain
    **
    **************************************************************************/
    class SimpleTransmissionGroups : public TransmissionGroupsBase
    {
    public:
        SimpleTransmissionGroups();

    protected:

        // ITransmissionGroups implementation
        virtual void AddProperty(const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix, const string& route);
        virtual void Build(const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, int numberOfStrains, int numberOfSubstrains = 1);
        virtual void GetGroupMembershipForProperties(const RouteList_t& route, const tProperties* properties, TransmissionGroupMembership_t* membershipOut ) const;
        virtual void UpdatePopulationSize(const TransmissionGroupMembership_t* transmissionGroupMembership, float size_changes, float mc_weight);
        virtual void DepositContagion(const IStrainIdentity& strain, float amount, const TransmissionGroupMembership_t* transmissionGroupMembership);
        virtual void ExposeToContagion(IInfectable* candidate, const TransmissionGroupMembership_t* transmissionGroupMembership, float deltaTee) const;
        virtual void CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership);
        virtual void EndUpdate(float infectivityCorrection);
        virtual float GetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership);

        // Implementation details
        void AddPropertyValueListToPropertyToValueMap( const string& route, const string& property, const PropertyValueList_t& values );
        void BuildRouteScalingMatrices( void );
        void StoreRouteDecayValues( const RouteToContagionDecayMap_t& contagionDecayRatesByRoute );
        void AllocateAccumulators();

        string transmissionRouteName;
        PropertyToValuesMap_t propertyNameToValuesMap;
        ScalingMatrix_t scalingMatrix;
        float contagionDecayRate;
        ContagionAccumulator_t shedContagion;
        ContagionAccumulator_t currentContagion;
        ContagionAccumulator_t forceOfInfection;
        float populationSize;
        unsigned int max_index;
    };
}
