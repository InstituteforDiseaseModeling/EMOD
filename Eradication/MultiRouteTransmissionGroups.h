/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "TransmissionGroupsBase.h"

namespace Kernel
{
    /**************************************************************************
    **
    ** MultiRouteTransmissionGroups support heterogeneous intra-node transmission with:
    **    - one or more transmission routes
    **    - a single major strain (antigen id)
    **    - a single substrain
    **************************************************************************/
    class MultiRouteTransmissionGroups : protected TransmissionGroupsBase
    {
    public:
        MultiRouteTransmissionGroups();

    protected:

        typedef map<RouteIndex, PropertyToValuesMap_t> RouteToPropertiesMap_t;
        RouteToPropertiesMap_t routeIndexToPropertiesMap;

        typedef map<const string, RouteIndex> RouteToRouteIndexMap_t;
        RouteToRouteIndexMap_t routeNameToIndexMap;             // Map route name (external) to route index (internal)
        vector<string> routeNames;                              // Reverse mapping

        typedef map<const string, string> PropertyToRouteMap_t;
        PropertyToRouteMap_t propertyNameToRouteNameMap;        // Used to determine group membership

        vector<ScalingMatrix_t> scalingMatrices;                // Indexed by route
        vector<float> contagionDecayRates;                      // Indexed by route

        vector<ContagionAccumulator_t> shedContagionByRoute;    // Indexed by route and group
        vector<ContagionAccumulator_t> currentContagionByRoute; // Indexed by route and group
        vector<ContagionAccumulator_t> infectionRateByRoute;    // Indexed by route and group

        vector<float> populationSizeByRoute;                   // Group population size for people default to the group. Indexed by route, and group

        void AddPropertyValueListToRoutePropertyToValueMap( const string& route, const string& property, const PropertyValueList_t& values );
        void AddRoute( const string& route );
        void AddRouteToPropertyToRouteMap( const string& property, const string& route );
        inline int getRouteCount()  const { return routeIndexToPropertiesMap.size(); }
        void BuildRouteScalingMatrices( int routeCount );
        void StoreRouteDecayValues( int routeCount, const RouteToContagionDecayMap_t& contagionDecayRatesByRoute );
        void AllocateAccumulators( int routeCount );

        inline int getGroupCountForRoute(int routeIndex) { return scalingMatrices[routeIndex].size(); }

    private:

        // ITransmissionGroups implementation
        virtual void AddProperty(const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix, const string& route);
        virtual void Build(const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, int numberOfStrains = 1, int numberOfSubstrains = 1);
        virtual void GetGroupMembershipForProperties(RouteList_t& route, const tProperties* properties, TransmissionGroupMembership_t* membershipOut ) const;
        virtual void UpdatePopulationSize(const TransmissionGroupMembership_t* transmissionGroupMembership, float size_changes, float mc_weight);
        virtual float GetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership);
        virtual void DepositContagion(const StrainIdentity* strain, float amount, const TransmissionGroupMembership_t* transmissionGroupMembership);
        virtual void ExposeToContagion(IInfectable* candidate, const TransmissionGroupMembership_t* transmissionGroupMembership, float deltaTee) const;
        virtual void CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership);
        virtual void EndUpdate(float infectivityCorrection);
    };
}
