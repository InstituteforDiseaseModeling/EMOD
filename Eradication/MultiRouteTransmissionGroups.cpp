/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Debug.h"
#include "Exceptions.h"
#include "MultiRouteTransmissionGroups.h"
#include "Log.h"

SETUP_LOGGING( "MultiRouteTransmissionGroups" )

static map< unsigned int, Kernel::TransmissionRoute::Enum > routeIndex2EnumMap;

namespace Kernel
{
    MultiRouteTransmissionGroups::MultiRouteTransmissionGroups()
        : routeIndexToPropertiesMap()
        , routeNameToIndexMap()
        , routeNames()
        , propertyNameToRouteNameMap()
        , scalingMatrices()
        , contagionDecayRates()
        , shedContagionByRoute()
        , currentContagionByRoute()
        , infectionRateByRoute()
        , populationSizeByRoute()
    {
    }

    void MultiRouteTransmissionGroups::AddProperty( const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix, const string& route )
    {
        LOG_DEBUG_F( "Adding property %s for route %s\n", property.c_str(), route.c_str() );
        CheckForDuplicatePropertyName(property);
        CheckForValidValueListSize(values);
        CheckForValidScalingMatrixSize(scalingMatrix, values);

        AddPropertyValueListToRoutePropertyToValueMap(route, property, values);
        AddScalingMatrixToPropertyToMatrixMap(property, scalingMatrix);
        AddRouteToPropertyToRouteMap(property, route);
    }

    // This is the public method for configurations that have a non-trivial test of properties and scaling matrix
    void MultiRouteTransmissionGroups::AddPropertyValueListToRoutePropertyToValueMap( const string& route, const string& property, const PropertyValueList_t& values )
    {
        // It's ok to add a new route here, but don't delegate to another function because we don't want to 
        // create impression that this is a legitimate solution for other code paths.
        if( routeNameToIndexMap.find( route ) == routeNameToIndexMap.end() )
        {
            routeNameToIndexMap.insert( make_pair( route, int(routeNameToIndexMap.size()) ) );
        }

        RouteIndex routeIndex = routeNameToIndexMap.at(route);
        if (routeIndexToPropertiesMap.find(routeIndex) == routeIndexToPropertiesMap.end())
        {
            // Create because it doesn't exist.
            PropertyToValuesMap_t emptyMap;
            routeIndexToPropertiesMap[routeIndex] = emptyMap;
        }

        PropertyToValuesMap_t& propertyValueMap = routeIndexToPropertiesMap.at(routeIndex);
        propertyValueMap[property] = values;
    }

    // This is the internal method for configurations that have a trivial property and scaling matrix
    // setup and just know about a route; they just call Build with the route and Build calsl AddRoute
    // for them.
    void MultiRouteTransmissionGroups::AddRoute( const string& route )
    {
        LOG_DEBUG_F( "Adding route %s\n", route.c_str() );
        routeNameToIndexMap.insert( make_pair( route, int(routeNameToIndexMap.size()) ) );
        routeNames.push_back(route); // note: AddProperty above doesn't populate routeNames because only VectorTransmissionGroups uses it and that goes through this path.

        // Do an implicit AddProperty here. Motivated by polio.
        if (propertyNameToMatrixMap.find(route) == propertyNameToMatrixMap.end())
        {
            MatrixRow_t rowOne(1);
            rowOne[0] = 1.0f;
            ScalingMatrix_t scalingMatrix;
            scalingMatrix.push_back(rowOne);
            PropertyValueList_t valueList;
            valueList.push_back( "Default" );
            AddProperty(route, valueList, scalingMatrix, route);
        }
        LOG_DEBUG_F( "Adding route %s: now routeNamesToIndexMap.size() = %d, routeNames.size() = %d\n", route.c_str(), routeNameToIndexMap.size(), routeNames.size() );
    }

    void MultiRouteTransmissionGroups::AddRouteToPropertyToRouteMap( const string& property, const string& route )
    {
        // We'll need this to map property:value pairs for group membership
        propertyNameToRouteNameMap[property] = route;
    }

    void MultiRouteTransmissionGroups::Build( const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, int numberOfStrains, int numberOfSubstrains /*= 1*/ )
    {
        for (auto& entry : contagionDecayRatesByRoute)
        {
            auto& routeName = entry.first;
            AddRoute( routeName );
        }

        int routeCount = getRouteCount();
        BuildRouteScalingMatrices(routeCount);
        StoreRouteDecayValues(routeCount, contagionDecayRatesByRoute);
        AllocateAccumulators(routeCount);

        if(routeIndex2EnumMap.size() == 0 )
        {
            routeIndex2EnumMap.insert( std::make_pair( 0, Kernel::TransmissionRoute::TRANSMISSIONROUTE_CONTACT ) );
            routeIndex2EnumMap.insert( std::make_pair( 1, Kernel::TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL ) );
        }
    }

    void MultiRouteTransmissionGroups::BuildRouteScalingMatrices( int routeCount )
    {
        LOG_DEBUG_F( "BuildRouteScalingMatrices for %d routes\n", routeCount );
        release_assert( routeCount );
        scalingMatrices.resize(routeCount);

        for (int iRoute = 0; iRoute < routeCount; iRoute++)
        {
            ScalingMatrix_t cumulativeMatrix;
            InitializeCumulativeMatrix(cumulativeMatrix);

            if( routeIndexToPropertiesMap.find( iRoute ) == routeIndexToPropertiesMap.end() )
            {
                throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "routeIndexToPropertiesMap", std::to_string(iRoute).c_str() );
            }
            PropertyToValuesMap_t& propertyToValueMap = routeIndexToPropertiesMap[iRoute];

            // For each property, aggregate the propertyMatrix with the cumulative scaling matrix
            for (const auto& pair : propertyToValueMap)
            {
                const string& propertyName = pair.first;
                const PropertyValueList_t& valueList = pair.second;
                AddPropertyValuesToValueToIndexMap(propertyName, valueList, cumulativeMatrix.size());
                if( propertyNameToMatrixMap.find( propertyName ) == propertyNameToMatrixMap.end() )
                {
                    throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "propertyNameToMatrixMap", propertyName.c_str() );
                }
                AggregatePropertyMatrixWithCumulativeMatrix(propertyNameToMatrixMap[propertyName], cumulativeMatrix);
            }

            scalingMatrices[iRoute] = cumulativeMatrix;
        }
    }

    void MultiRouteTransmissionGroups::AllocateAccumulators( int routeCount )
    {
        // We need a shed and current contagion accumulator for each route
        shedContagionByRoute.resize(routeCount);
        currentContagionByRoute.resize(routeCount);
        populationSizeByRoute.resize(routeCount);
        infectionRateByRoute.resize(routeCount);

        for (RouteIndex routeIndex = 0; routeIndex < routeCount; routeIndex++)
        {
            int groupCount = getGroupCountForRoute(routeIndex);
            shedContagionByRoute[routeIndex].resize(groupCount);
            currentContagionByRoute[routeIndex].resize(groupCount);
            infectionRateByRoute[routeIndex].resize(groupCount);
        }
    }

    void MultiRouteTransmissionGroups::StoreRouteDecayValues( int routeCount, const RouteToContagionDecayMap_t& contagionDecayRatesByRoute )
    {
        release_assert( routeCount );
        contagionDecayRates.resize(routeCount, 1.0f);
        release_assert( routeNameToIndexMap.size() == contagionDecayRatesByRoute.size() );
        for (const auto& entry : contagionDecayRatesByRoute)
        {
            const string& routeName = entry.first;
            if( routeNameToIndexMap.find( routeName ) == routeNameToIndexMap.end() )
            {
                throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "routeNameToIndexMap", routeName.c_str() );
            }
            RouteIndex routeIndex = routeNameToIndexMap[routeName];
            float decayRate = entry.second;

            contagionDecayRates[routeIndex] = decayRate;
            LOG_DEBUG_F( "Stored decay rate %f for route %s\n", decayRate, routeName.c_str() );
        }
    }

    void
    MultiRouteTransmissionGroups::GetGroupMembershipForProperties( const RouteList_t& route, const tProperties* properties, TransmissionGroupMembership_t* membershipOut ) const
    {
        //only initialize routes which are in specified routes

        unsigned int idx = 0;
        for (const auto& entry : routeNameToIndexMap)
        {
            const std::string& routeNameFromMap = entry.first;
            LOG_VALID_F("Looking at route name %s.\n", routeNameFromMap.c_str());
            for (int iRoute = 0; iRoute < route.size(); iRoute++)
            {
                const string& routeName = route[iRoute];
                if (routeNameFromMap == routeName)
                {
                    LOG_VALID_F( "Setting tx group membership for index %d to %d (route index): routeName (passed in) = %s, routeNameFromMap (from loop) is same.\n", idx, iRoute, routeName.c_str() );
                    (*membershipOut)[idx] = 0; // iRoute;
                }
            }
            idx++;
        }
        
        for (const auto& entry : (*properties))
        {
            const string& propertyName = entry.first;
            if (propertyNameToRouteNameMap.find(propertyName) != propertyNameToRouteNameMap.end())
            {
                const string& routeName = propertyNameToRouteNameMap.at(propertyName);
                RouteIndex routeIndex = routeNameToIndexMap.at(routeName);
                if ((*membershipOut).find(routeIndex) != (*membershipOut).end())
                {
                    const string& propertyValue = entry.second;
                    LOG_DEBUG_F( "Increasing tx group membership for (route) index %d (property name=%s, value=%s) by %d.\n", routeIndex, propertyName.c_str(), propertyValue.c_str(), (propertyValueToIndexMap.at(propertyName).at(propertyValue) ) );
                    (*membershipOut)[routeIndex] += (propertyValueToIndexMap.at(propertyName).at(propertyValue));
                }
            }
        }
        //LOG_DEBUG_F( "membership returning %p\n", membershipOut );
    }

    void MultiRouteTransmissionGroups::UpdatePopulationSize(const TransmissionGroupMembership_t* transmissionGroupMembership, float size_changes, float mc_weight)
    {
        release_assert( transmissionGroupMembership );
        for (const auto& entry : (*transmissionGroupMembership))
        {
            RouteIndex routeIndex = entry.first;
            // GroupIndex groupIndex = entry.second;
            populationSizeByRoute[routeIndex] += size_changes * mc_weight;
        }
    }

    float MultiRouteTransmissionGroups::GetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        float totalContagion = 0;
        for (const auto& entry : (*transmissionGroupMembership))
        {
            RouteIndex routeIndex = entry.first;
            GroupIndex groupIndex = entry.second;
            LOG_DEBUG_F( "Getting infectionRateByRoute: route index %d and group index %d: %f\n", routeIndex, groupIndex, infectionRateByRoute[routeIndex][groupIndex] );

            //totalContagion += shedContagionByRoute[routeIndex][groupIndex];
            totalContagion += infectionRateByRoute[routeIndex][groupIndex];
        }

        return totalContagion;
    }

    void MultiRouteTransmissionGroups::DepositContagion( const IStrainIdentity& /* strain */, float amount, const TransmissionGroupMembership_t* transmissionGroupMembership )
    {
        for (const auto& entry : (*transmissionGroupMembership))
        {
            RouteIndex routeIndex = entry.first;
            GroupIndex groupIndex = entry.second;
            shedContagionByRoute[routeIndex][groupIndex] += amount;
        }
    }

    void MultiRouteTransmissionGroups::ExposeToContagion( IInfectable* candidate, const TransmissionGroupMembership_t* transmissionGroupMembership, float deltaTee ) const
    {
        float infectionRateForGroups = 0.0f;

        for (const auto& entry : (*transmissionGroupMembership))
        {
            RouteIndex routeIndex = entry.first;
            GroupIndex groupIndex = entry.second;
            infectionRateForGroups += infectionRateByRoute[routeIndex][groupIndex];
        }

        if ((infectionRateForGroups > 0) && (candidate != nullptr))
        {
            for (const auto& entry : (*transmissionGroupMembership))
            {
                auto routeIndex  = entry.first;
                auto groupIndex  = entry.second;
                ContagionPopulationImpl contagionPopulation(infectionRateForGroups);
                auto txroute = routeIndex2EnumMap.at( routeIndex ); 
                candidate->Expose(static_cast<IContagionPopulation*>(&contagionPopulation), deltaTee, txroute );
            }
        }
    }

    void MultiRouteTransmissionGroups::CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        for (const auto& entry : (*transmissionGroupMembership))
        {
            RouteIndex routeIndex = entry.first;
            GroupIndex groupIndex = entry.second;
            shedContagionByRoute[routeIndex][groupIndex] *= infectivityCorrection;
        }
    }

    void MultiRouteTransmissionGroups::EndUpdate(float infectivityCorrection)
    {
        LOG_VALID_F( "%s\n", __FUNCTION__ );
        int routeCount = shedContagionByRoute.size();
        for (int iRoute = 0; iRoute < routeCount; iRoute++)
        {
            float decay = 1.0f - contagionDecayRates[iRoute];
            ScalingMatrix_t& scalingMatrix = scalingMatrices[iRoute];
            vector<float>& currentContagion = currentContagionByRoute[iRoute];
            LOG_DEBUG_F( "Applying decay rate of %f on route %d to accumulation of %f\n", decay, iRoute, currentContagionByRoute[ iRoute ][0] );
            int groupCount = scalingMatrix.size();
            for (int iSink = 0; iSink < groupCount; iSink++)
            {
                vector<float>& shedContagion = shedContagionByRoute[iRoute];
                MatrixRow_t& row = scalingMatrix[iSink];
                float sum = VectorDotProduct(shedContagion, row);
                sum *= infectivityCorrection;
                currentContagion[iSink] *= decay;
                currentContagion[iSink] += sum;
                LOG_DEBUG_F("Adding %f to %f contagion [route:%d,group:%d]\n", sum, currentContagion[iSink], iRoute, iSink);
            }

            float populationForRoute = populationSizeByRoute[iRoute];
            vector<float>& infectionRateForRoute = infectionRateByRoute[iRoute];
            if (populationForRoute > 0)
            {
                memcpy(infectionRateForRoute.data(), currentContagion.data(), groupCount * sizeof(float));
                VectorScalarMultiplyInPlace(infectionRateForRoute, 1.0f/populationForRoute);
            }
            else
            {
                memset(infectionRateForRoute.data(), 0, groupCount * sizeof(float));
            }

            LOG_VALID_F( "foi for route %d = %f\n", iRoute, infectionRateForRoute[0] );

            // Reset the shed contagion accumulator for the route
            memset(shedContagionByRoute[iRoute].data(), 0, shedContagionByRoute[iRoute].size() * sizeof(float));

        }
    }
}
