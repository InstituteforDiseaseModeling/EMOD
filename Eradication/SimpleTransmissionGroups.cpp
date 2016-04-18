/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "SimpleTransmissionGroups.h"
#include "Exceptions.h"
#include "Debug.h"  // release_assert
#include "Log.h"

static const char* _module = "SimpleTransmissionGroups";

namespace Kernel
{
    SimpleTransmissionGroups::SimpleTransmissionGroups()
        : transmissionRouteName("")
        , propertyNameToValuesMap()
        , scalingMatrix()
        , contagionDecayRate(1.0f)
        , shedContagion()
        , currentContagion()
        , forceOfInfection()
        , populationSize(0.0f)
    {
        LOG_DEBUG( "Creating SimpleTransmissionGroups object.\n" );
    }

    void SimpleTransmissionGroups::AddProperty( const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix, const string& route )
    {
        LOG_DEBUG_F( "Adding Property for property %s and route %s.\n", property.c_str(), route.c_str() );
        CheckForDuplicatePropertyName(property);
        CheckForValidValueListSize(values);
        CheckForValidScalingMatrixSize(scalingMatrix, values);

        AddPropertyValueListToPropertyToValueMap(route, property, values);
        AddScalingMatrixToPropertyToMatrixMap(property, scalingMatrix);
    }

    void SimpleTransmissionGroups::AddPropertyValueListToPropertyToValueMap( const string& route, const string& property, const PropertyValueList_t& values )
    {
        if (transmissionRouteName.length() == 0)
        {
            transmissionRouteName = string(route);
        }
        else if (route != transmissionRouteName)
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "SimpleTransmissionGroups only supports one route.");
        }

        propertyNameToValuesMap[property] = values;
        LOG_DEBUG_F( "Adding list of %d values to propertyNameToValuesMap for key/property %s\n", values.size(), property.c_str() );
    }

    void SimpleTransmissionGroups::Build( const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, int numberOfStrains, int numberOfSubstrains )
    {
        //CheckForValidStrainListSize(strains);
        BuildRouteScalingMatrices();
        StoreRouteDecayValues(contagionDecayRatesByRoute);
        AllocateAccumulators();
    }

    void SimpleTransmissionGroups::BuildRouteScalingMatrices( void )
    {
        InitializeCumulativeMatrix(scalingMatrix);

        // For each property, aggregate the propertyMatrix with the cumulative scaling matrix
        for (const auto& entry : propertyNameToValuesMap)
        {
            const string& propertyName = entry.first;
            const PropertyValueList_t& valueList = entry.second;
            AddPropertyValuesToValueToIndexMap(propertyName, valueList, scalingMatrix.size());
            if( propertyNameToMatrixMap.find( propertyName ) == propertyNameToMatrixMap.end() )
            {
                throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "propertyNameToMatrixMap", propertyName.c_str() );
            }
            AggregatePropertyMatrixWithCumulativeMatrix(propertyNameToMatrixMap[propertyName], scalingMatrix);
        }
    }

    void SimpleTransmissionGroups::StoreRouteDecayValues( const RouteToContagionDecayMap_t& contagionDecayRatesByRoute )
    {
        if (contagionDecayRatesByRoute.find(transmissionRouteName) != contagionDecayRatesByRoute.end())
        {
            contagionDecayRate = contagionDecayRatesByRoute.at(transmissionRouteName);
        }
    }

    void SimpleTransmissionGroups::AllocateAccumulators()
    {
        int bucketCount = scalingMatrix.size();
        shedContagion.resize(bucketCount);
        memset(shedContagion.data(), 0, bucketCount * sizeof(float));
        currentContagion.resize(bucketCount);
        memset(currentContagion.data(), 0, bucketCount * sizeof(float));
        forceOfInfection.resize(bucketCount);
        memset(forceOfInfection.data(), 0, bucketCount * sizeof(float));
    }

    void SimpleTransmissionGroups::GetGroupMembershipForProperties( const RouteList_t& route, const tProperties* properties, TransmissionGroupMembership_t* membershipOut ) const
    {
        (*membershipOut)[0] = GroupIndex(0); // map route 0 to index 0
        std::ostringstream* msg = nullptr;
        if (LOG_LEVEL(DEBUG)) 
        {
            msg = new std::ostringstream();
            *msg << "(fn=GetGroupMembershipForProperties) ";
        }

        for (const auto& entry : (*properties))
        {
            const string& propertyName = entry.first;
            const string& propertyValue = entry.second;

            if (propertyValueToIndexMap.find(propertyName) != propertyValueToIndexMap.end())
            {
                if( propertyValueToIndexMap.at(propertyName).find( propertyValue ) != propertyValueToIndexMap.at(propertyName).end() )
                {
                    (*membershipOut)[0] += propertyValueToIndexMap.at(propertyName).at(propertyValue);
                }
                else
                {
                    throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, (std::string("propertyValueToIndexMap[")+propertyName+"]").c_str(), propertyValue.c_str() );
                }
            }
            if (LOG_LEVEL(DEBUG))
            {
                release_assert( msg ); // ensure someone doesn't change logic above so this is not allocated.
                *msg << propertyName << "=" << propertyValue << ", ";
            }
        }
        if (LOG_LEVEL(DEBUG))
        {
            release_assert( msg ); // ensure someone doesn't change logic above so this is not allocated.
            *msg << "=> Group index for route 0 is " << (*membershipOut)[0] << std::endl;
            LOG_DEBUG( msg->str().c_str() );
            delete msg;
        }
    }

    void SimpleTransmissionGroups::UpdatePopulationSize(const TransmissionGroupMembership_t* transmissionGroupMembership, float size_changes, float mc_weight)
    {
        float delta = size_changes * mc_weight;
        populationSize += delta;
        LOG_DEBUG_F( "Increased group population to %f with addition of %f\n", populationSize, delta );
    }

    float SimpleTransmissionGroups::GetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        GroupIndex groupIndex = transmissionGroupMembership->at(0);
        return currentContagion[groupIndex];
    }

    void SimpleTransmissionGroups::DepositContagion( const StrainIdentity* strain, float amount, const TransmissionGroupMembership_t* transmissionGroupMembership )
    {
        GroupIndex groupIndex = transmissionGroupMembership->at(0);
        shedContagion[groupIndex] += amount;
        if (amount > 0)
        {
            LOG_DEBUG_F("deposited %f amount of contagion to group %d\n", amount, groupIndex);
        }
    }

    void SimpleTransmissionGroups::ExposeToContagion( IInfectable* candidate, const TransmissionGroupMembership_t* transmissionGroupMembership, float deltaTee ) const
    {
        LOG_DEBUG_F( "%s\n", __FUNCTION__ );
        GroupIndex groupIndex = transmissionGroupMembership->at(0);
        float groupInfectionRate = forceOfInfection[groupIndex];
        ContagionPopulationImpl contagionPopulation(groupInfectionRate);

        if (groupInfectionRate > 0)
        {
            if (candidate != nullptr)
            {
                candidate->Expose(static_cast<IContagionPopulation*>(&contagionPopulation), deltaTee, TransmissionRoute::TRANSMISSIONROUTE_ALL);
            }
        }
    }

    void SimpleTransmissionGroups::CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        GroupIndex groupIndex = transmissionGroupMembership->at(0);
        shedContagion[groupIndex] *= infectivityCorrection;
    }

    /***********************************************************************************************

    Given beta matrix, B[i,j] (i-sink, j-source) and Y[j], contagion shed from group j,

    calculate R[i], the rate of infection per susceptible in group i, where

    R[i] = (1/N) * sum over j of B[i,j] * Y[j]

    ************************************************************************************************/

    void SimpleTransmissionGroups::EndUpdate(float infectivityCorrection)
    {
        // Distribute contagion shed to destination groups
        float decay = 1.0f - contagionDecayRate;
        int groupCount = scalingMatrix.size();
        for (int iSink = 0; iSink < groupCount; iSink++)
        {
            MatrixRow_t& row = scalingMatrix[iSink];
            float sum = VectorDotProduct(shedContagion, row);
            sum *= infectivityCorrection;
            currentContagion[iSink] *= decay;
            currentContagion[iSink] += sum;
        }

        if (populationSize > 0)
        {
            memcpy(forceOfInfection.data(), currentContagion.data(), groupCount * sizeof(float));
            VectorScalarMultiplyInPlace(forceOfInfection, 1.0f/populationSize);
        }
        else
        {
            memset(forceOfInfection.data(), 0, groupCount * sizeof(float));
        }

        memset(shedContagion.data(), 0, groupCount * sizeof(float));
    }
}
