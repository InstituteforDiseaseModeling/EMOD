/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "StrainAwareTransmissionGroups.h"
#include "Exceptions.h"

// These includes are required to bring in randgen
#include "Environment.h"
//#include "Contexts.h"
#include "RANDOM.h"

#include "Log.h"
#include "Debug.h"
#include "StrainIdentity.h"
#include "SimulationConfig.h"
#include <numeric>

SETUP_LOGGING( "StrainAwareTransmissionGroups" )

namespace Kernel
{
    StrainAwareTransmissionGroups::StrainAwareTransmissionGroups( RANDOMBASE* prng )
        : pRNG( prng )
        , propertyToValuesMap()
        , scalingMatrix()
        , contagionDecayRate( 1.0f )
        , populationSize( 0.0f )
        , populationSizeByGroup()
        , antigenCount(0)
        , substrainCount(0)
        , normalizeByTotalPopulation(true)
        , antigenWasShed()
        , substrainWasShed()
        , newlyDepositedContagionByAntigenAndGroup()
        , currentContagionByAntigenAndSourceGroup()
        , currentContagionByAntigenAndDestinationGroup()
        , forceOfInfectionByAntigenAndGroup()
        , newContagionByAntigenGroupAndSubstrain()
        , currentContagionByAntigenSourceGroupAndSubstrain()
        , currentContagionByAntigenDestinationGroupAndSubstrain()
        , forceOfInfectionByAntigenGroupAndSubstrain()
        , tag("contact")
    {
    }

    void StrainAwareTransmissionGroups::AddProperty( const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix )
    {
        LOG_DEBUG_F( "Adding property %s\n", property.c_str() );
        checkForDuplicatePropertyName(property);
        checkForValidValueListSize(values);
        checkForValidScalingMatrixSize(scalingMatrix, values);

        addPropertyValueListToPropertyToValueMap(property, values);
        addScalingMatrixToPropertyToMatrixMap(property, scalingMatrix);
    }

    void StrainAwareTransmissionGroups::addPropertyValueListToPropertyToValueMap( const string& property, const PropertyValueList_t& values )
    {
        propertyToValuesMap[property] = values;
    }

    void StrainAwareTransmissionGroups::buildScalingMatrix( void )
    {
        LOG_DEBUG_F( "%s\n", __FUNCTION__ );

        ScalingMatrix_t cumulativeMatrix;
        InitializeCumulativeMatrix(cumulativeMatrix);

        // For each property, aggregate the propertyMatrix with the cumulative scaling matrix
        for (const auto& pair : propertyToValuesMap)
        {
            const string& propertyName = pair.first;
            const PropertyValueList_t& valueList = pair.second;
            addPropertyValuesToValueToIndexMap(propertyName, valueList, cumulativeMatrix.size());

            AggregatePropertyMatrixWithCumulativeMatrix(propertyNameToMatrixMap[propertyName], cumulativeMatrix);
        }

        scalingMatrix = cumulativeMatrix;
    }

    // EVERYTHING UP TO HERE WAS SETUP. AFTER HERE IS RUNTIME STUFF.

    /* 
     * The goal here is to create and return a map of route indices to group indices. So each route,
     * e.g., environmental and contact, represented by an id, say 1 and 0, will map to property "ids",
     * aka group ids, like 0-10.
     */
    void
    StrainAwareTransmissionGroups::GetGroupMembershipForProperties( const tProperties& properties, TransmissionGroupMembership_t& membershipOut ) const
    {
        // Start at 0. Will, potentially, modify below based on property values and their index offsets.
        membershipOut.group = 0;

        for( const auto& entry : properties )
        {
            const string& propertyName = entry.first;
            if( propertyNameToMatrixMap.find( propertyName ) != propertyNameToMatrixMap.end() )
            {
                if( propertyValueToIndexMap.find( propertyName ) != propertyValueToIndexMap.end() &&
                    propertyValueToIndexMap.at( propertyName ).find( entry.second ) != propertyValueToIndexMap.at( propertyName ).end() )
                {
                    const string& propertyValue = entry.second;
                    size_t offset = propertyValueToIndexMap.at( propertyName ).at( propertyValue );
                    LOG_VALID_F( "Increasing/setting tx group membership for (route) index 0 by %d.\n", offset );
                    LOG_DEBUG_F( "Increasing tx group membership for (route) index 0 (property name=%s, value=%s) by %d.\n", propertyName.c_str(), propertyValue.c_str(), offset );
                    membershipOut.group += offset;
                }
                else
                {
                    // TBD: Actually there are 2 possible map failures and we should add code to be specific so we get right one.
                    throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "propertyValueToIndexMap", propertyName.c_str() );
                }
            }
            else
            {
                LOG_DEBUG_F( "Property %s not used in HINT configuration.\n", propertyName.c_str() );
            }
        }
        //LOG_DEBUG_F( "membership returning %p\n", membershipOut );
    }

    void StrainAwareTransmissionGroups::UpdatePopulationSize(const TransmissionGroupMembership_t& membership, float size_changes, float mc_weight)
    {
        float delta = size_changes * mc_weight;
        populationSize += delta;
        populationSizeByGroup[membership.group] += delta;
    }

    void StrainAwareTransmissionGroups::Build(float contagionDecayRate, int numberOfStrains, int numberOfSubstrains)
    {
        buildScalingMatrix();
        this->contagionDecayRate = contagionDecayRate;
        allocateAccumulators(numberOfStrains, numberOfSubstrains);

        LOG_DEBUG_F("Built %d groups with %d strains and %d substrains.\n", getGroupCount(), numberOfStrains, numberOfSubstrains);
    }

    void StrainAwareTransmissionGroups::allocateAccumulators( NaturalNumber numberOfStrains, NaturalNumber numberOfSubstrains )
    {
        LOG_VALID( "AllocateAccumulators called.\n" );
        antigenCount = numberOfStrains;
        substrainCount = numberOfSubstrains;

        antigenWasShed.resize(antigenCount);
        substrainWasShed.resize(antigenCount);

        newlyDepositedContagionByAntigenAndGroup.resize(antigenCount);
        currentContagionByAntigenAndSourceGroup.resize(antigenCount);
        currentContagionByAntigenAndDestinationGroup.resize(antigenCount);
        forceOfInfectionByAntigenAndGroup.resize(antigenCount);
        newContagionByAntigenGroupAndSubstrain.resize(antigenCount);
        currentContagionByAntigenSourceGroupAndSubstrain.resize(antigenCount);
        currentContagionByAntigenDestinationGroupAndSubstrain.resize(antigenCount);
        forceOfInfectionByAntigenGroupAndSubstrain.resize(antigenCount);
        for (size_t iAntigen = 0; iAntigen < antigenCount; ++iAntigen)
        {
            size_t groupCount = getGroupCount();
            newlyDepositedContagionByAntigenAndGroup[iAntigen].resize(groupCount);
            currentContagionByAntigenAndSourceGroup[iAntigen].resize(groupCount);
            currentContagionByAntigenAndDestinationGroup[iAntigen].resize(groupCount);
            forceOfInfectionByAntigenAndGroup[iAntigen].resize(groupCount);
            newContagionByAntigenGroupAndSubstrain[iAntigen].resize(groupCount);
            currentContagionByAntigenSourceGroupAndSubstrain[iAntigen].resize(groupCount);
            currentContagionByAntigenDestinationGroupAndSubstrain[iAntigen].resize(groupCount);
            forceOfInfectionByAntigenGroupAndSubstrain[iAntigen].resize(groupCount);
        }

        populationSizeByGroup.resize(getGroupCount());
    }

    void StrainAwareTransmissionGroups::DepositContagion(const IStrainIdentity& strain, float amount, TransmissionGroupMembership_t membership)
    {
        if ( amount > 0 )
        {
            int iAntigen    = strain.GetAntigenID();
            int substrainId = strain.GetGeneticID();
            // REMOVE? LOG_DEBUG_F("%s: iAntigen = %d, substrainId = %d\n", __FUNCTION__, iAntigen, substrainId);

            if ( iAntigen >= antigenCount )
            {
                ostringstream msg;
                msg << "Strain antigen ID (" << iAntigen << ") >= configured number of strains (" << antigenCount << ").\n";
                throw new OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str(), float(iAntigen), float(antigenCount) );
            }

            antigenWasShed[iAntigen]                = true;
            substrainWasShed[iAntigen].insert(substrainId);

            GroupIndex iGroup = membership.group;
            newlyDepositedContagionByAntigenAndGroup[iAntigen][iGroup] += amount;
            newContagionByAntigenGroupAndSubstrain[iAntigen][iGroup][substrainId] += amount;

            LOG_VALID_F( "(%s) DepositContagion (antigen = %d, route = 0, group = %d [substrain = %d]) increased by %f\n",
                            tag.c_str(),
                            iAntigen,
                            iGroup,
                            substrainId,
                            amount );
        }
    }

    void StrainAwareTransmissionGroups::ExposeToContagion(IInfectable* candidate, TransmissionGroupMembership_t membership, float deltaTee, TransmissionRoute::Enum txRoute) const
    {
        //LOG_DEBUG_F( "ExposeToContagion\n" );
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            float forceOfInfection = 0.0f;
            const ContagionAccumulator_t& forceOfInfectionByGroup = forceOfInfectionByAntigenAndGroup[iAntigen];

            size_t iGroup = membership.group;
            forceOfInfection = forceOfInfectionByGroup[iGroup];

            if ((forceOfInfection > 0) && (candidate != nullptr))
            {
                LOG_DEBUG_F("ExposureToContagion: [Antigen:%d] Route:0, Group:%d, exposure qty = %f\n", iAntigen, iGroup, forceOfInfection );
                SubstrainPopulationImpl contagionPopulation(pRNG, iAntigen, forceOfInfection, forceOfInfectionByAntigenGroupAndSubstrain[iAntigen][iGroup]);
                candidate->Expose((IContagionPopulation*)&contagionPopulation, deltaTee, txRoute );
            }
        }
    }

    float StrainAwareTransmissionGroups::GetContagionByProperty(const IPKeyValue& property_value)
    {
        std::vector<size_t> indices;
        getGroupIndicesForProperty( property_value, propertyToValuesMap, indices );
        float total = 0.0f;
        for (size_t iAntigen = 0; iAntigen < antigenCount; ++iAntigen)
        {
            const auto& contagion = forceOfInfectionByAntigenAndGroup[iAntigen];
            total += std::accumulate(indices.begin(), indices.end(), 0.0f, [&](float init, size_t index) { return init + contagion[index]; });
        }

        return total;
    }

    void StrainAwareTransmissionGroups::CorrectInfectivityByGroup(float infectivityMultiplier, TransmissionGroupMembership_t membership)
    {
        // By antigen (substrains aggregated)
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            int iGroup = membership.group;
            LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d] Route:0, Group:%d, ContagionBefore = %f, infectivityMultiplier = %f\n", iAntigen, iGroup, newlyDepositedContagionByAntigenAndGroup[iAntigen][iGroup], infectivityMultiplier);
            newlyDepositedContagionByAntigenAndGroup[iAntigen][iGroup] *= infectivityMultiplier;
            LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d] Route:0, Group:%d, ContagionAfter = %f\n", iAntigen, iGroup, newlyDepositedContagionByAntigenAndGroup[iAntigen][iGroup]);
        }

        // By individual substrain
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            GroupSubstrainMap_t& shedAntigen = newContagionByAntigenGroupAndSubstrain[iAntigen];
            int iGroup = membership.group;
            for (auto& entry : shedAntigen[iGroup])
            {
                uint32_t substrainId = entry.first;
                LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d][Route:0][Group:%d][Substrain:%d], ContagionBefore = %f, infectivityMultiplier = %f\n", iAntigen, iGroup, substrainId, shedAntigen[iGroup][substrainId], infectivityMultiplier);
                entry.second *= infectivityMultiplier;
                LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d][Route:0][Group:%d][Substrain:%d], ContagionAfter  = %f\n", iAntigen, iGroup, substrainId, shedAntigen[iGroup][substrainId]);
            }
        }
    }

    void StrainAwareTransmissionGroups::EndUpdate( float infectivityMultiplier, float infectivityAddition )
    {
        LOG_VALID_F( "(%s) Enter (%s)\n", tag.c_str(), __FUNCTION__ );

        float additionalContagion = infectivityAddition;

        if ( (infectivityAddition != 0.0f) &&
             ((getGroupCount() > 1) || (antigenCount > 1) || (substrainCount > 1)) )
        {
            LOG_WARN_F( "StrainAwareTransmissionGroups::EndUpdate() infectivityAddition != 0 (%f actual), but one or more of # HINT groups (%d), antigen count (%d), or substrain count (%d) is > 1. Using 0 for additional contagion.\n",
                        infectivityAddition, getGroupCount(), antigenCount, substrainCount );
            additionalContagion = 0.0f;
        }

        for (size_t iAntigen = 0; iAntigen < antigenCount; ++iAntigen)
        {
            size_t groupCount = getGroupCount();
            for (size_t iGroup = 0; iGroup < groupCount; ++iGroup)
            {
                LOG_VALID_F( "(%s) New contagion (antigen = %d, route = 0, group = %d) = %f\n", tag.c_str(), iAntigen, iGroup, newlyDepositedContagionByAntigenAndGroup[iAntigen][iGroup] );
                for (const auto& entry : newContagionByAntigenGroupAndSubstrain[iAntigen][iGroup])
                {
                    LOG_VALID_F( "(%s) New contagion (antigen = %d, route = 0, group = %d, substrain = %d) = %f\n", tag.c_str(), iAntigen, iGroup, entry.first, entry.second );
                }
                LOG_VALID_F( "(%s) Current contagion [source] (antigen = %d, route = 0, group = %d) = %f\n", tag.c_str(), iAntigen, iGroup, currentContagionByAntigenAndSourceGroup[iAntigen][iGroup] );
                for (const auto& entry : currentContagionByAntigenSourceGroupAndSubstrain[iAntigen][iGroup])
                {
                    LOG_VALID_F( "(%s) Current contagion [source] (antigen = %d, route = 0, group = %d, substrain = %d) = %f\n", tag.c_str(), iAntigen, iGroup, entry.first, entry.second );
                }
            }
        }

        // For each antigen...
        for (size_t iAntigen = 0; iAntigen < antigenCount; ++iAntigen)
        {
            auto& refCurrentContagionForAntigenBySourceGroup                  = currentContagionByAntigenAndSourceGroup[iAntigen];
            auto& refCurrentContagionForAntigenByDestinationGroup             = currentContagionByAntigenAndDestinationGroup[iAntigen];
            auto& refCurrentContagionForAntigenBySourceGroupAndSubstrain      = currentContagionByAntigenSourceGroupAndSubstrain[iAntigen];
            auto& refCurrentContagionForAntigenByDestinationGroupAndSubstrain = currentContagionByAntigenDestinationGroupAndSubstrain[iAntigen];
            auto& refNewlyDepositedContagionForAntigenByGroup                 = newlyDepositedContagionByAntigenAndGroup[iAntigen];
            auto& refForceOfInfectionForAntigenByGroup                        = forceOfInfectionByAntigenAndGroup[iAntigen];
            auto& refNewContagionForAntigenByGroupAndSubstrain                = newContagionByAntigenGroupAndSubstrain[iAntigen];
            auto& refForceOfInfectionForAntigenByGroupAndSubstrain            = forceOfInfectionByAntigenGroupAndSubstrain[iAntigen];

            // Decay previously accumulated contagion and add in newly deposited contagion for each source group:
            float decayFactor = 1.0f - contagionDecayRate;
            LOG_VALID_F ( "(%s) Decay rate for route 0 = %f => decay factor = %f\n", tag.c_str(), contagionDecayRate, decayFactor );
            vectorScalarMultiply( refCurrentContagionForAntigenBySourceGroup, decayFactor );
            vectorElementAdd( refCurrentContagionForAntigenBySourceGroup, refNewlyDepositedContagionForAntigenByGroup );
            vectorScalarAdd( refCurrentContagionForAntigenBySourceGroup, additionalContagion );

            // We just added this to the current contagion accumulator. Clear it out.
            size_t groupCount = getGroupCount();
            memset( refNewlyDepositedContagionForAntigenByGroup.data(), 0, groupCount * sizeof(float) );

            // Current contagion (by group which deposited it) is up to date and new contagion by group is reset.

            // Update accumulated contagion by destination group:
            for (size_t iGroup = 0; iGroup < groupCount; ++iGroup)
            {
                const MatrixRow_t& betaVector = scalingMatrix[iGroup];
                float accumulatedContagion = vectorDotProduct( refCurrentContagionForAntigenBySourceGroup, betaVector );
                LOG_VALID_F("(%s) Adding %f to %f contagion [antigen:%d,route:0,group:%d]\n", tag.c_str(), accumulatedContagion, 0.0f, iAntigen, iGroup);
                refCurrentContagionForAntigenByDestinationGroup[iGroup] = accumulatedContagion;
            }

            // Current contagion (by receiving group) is up to date (based on current contagion from source groups).

            for (size_t iGroup = 0; iGroup < groupCount; ++iGroup)
            {
                // Update effective contagion (force of infection) by destination group:
                float population = (normalizeByTotalPopulation ? populationSize : populationSizeByGroup[iGroup]);
                float normalization = ((population == 0.0f) ? 0.0f : 1.0f/population);
                LOG_VALID_F( "(%s) Normalization (%s) for group %d is %f based on population %f\n", tag.c_str(), (normalizeByTotalPopulation ? "total population" : "group population"), iGroup, normalization, population );
                LOG_VALID_F( "(%s) Contagion for [antigen:%d,route:0] population scaled by %f\n", tag.c_str(), iAntigen, population);

                refForceOfInfectionForAntigenByGroup[iGroup] = refCurrentContagionForAntigenByDestinationGroup[iGroup]
                                                                * infectivityMultiplier
                                                                * normalization;
            }

            // Force of infection for each group (current contagion * correction * normalization) is up to date.

            for (size_t iGroup = 0; iGroup < groupCount; ++iGroup)
            {
                // Decay previously accumulated contagion and add in newly deposited contagion by substrain
                auto& refCurrentContagionForAntigenAndSourceGroupBySubstrain = refCurrentContagionForAntigenBySourceGroupAndSubstrain[iGroup];
                auto& refNewContagionForAntigenAndGroupBySubstrain           = refNewContagionForAntigenByGroupAndSubstrain[iGroup];

                // Decay previously accumulated contagion:
                if ( decayFactor > 0.0f )
                {
                    for (auto& entry : refCurrentContagionForAntigenAndSourceGroupBySubstrain)
                    {
                        // entry = pair<substrainId, contagion>
                        entry.second *= decayFactor;
                        // TODO - consider a threshold below which we stop tracking this strain (i.e. remove the entry from the map)
                    }
                }
                else
                {
                    refCurrentContagionForAntigenAndSourceGroupBySubstrain.clear();
                }

                // Current contagion (by source group) has been decayed.

                // Add in newly deposited contagion:
                for (auto& entry : refNewContagionForAntigenAndGroupBySubstrain)
                {
                    auto substrainId = entry.first;
                    auto contagion   = entry.second;
                    refCurrentContagionForAntigenAndSourceGroupBySubstrain[substrainId] += (contagion + additionalContagion);
                }

                // Current contagion, per substrain, (by source group) has been updated from new contagion, per substrain, (by source group)

                // We just added this to the current substrain contagion accumulator, clear it out.
                refNewContagionForAntigenAndGroupBySubstrain.clear();
            }

            for (size_t iGroup = 0; iGroup < groupCount; ++iGroup)
            {
                auto& refCurrentContagionForAntigenAndDestinationGroupBySubstrain = refCurrentContagionForAntigenByDestinationGroupAndSubstrain[iGroup];

                const MatrixRow_t& betaVector = scalingMatrix[iGroup];

                // We are going to transfer accumulated contagion to this structure, clear it out.
                refCurrentContagionForAntigenAndDestinationGroupBySubstrain.clear();

                // Update accumulated contagion, by substrain, indexed by destination group:
                for (size_t srcGroup = 0; srcGroup < groupCount; ++srcGroup)
                {
                    float beta = betaVector[srcGroup];
                    for (const auto& entry : refCurrentContagionForAntigenBySourceGroupAndSubstrain[srcGroup])
                    {
                        auto substrain = entry.first;
                        auto contagion = entry.second;
                        refCurrentContagionForAntigenAndDestinationGroupBySubstrain[substrain] += contagion * beta;
                    }
                }

                // Current contagion, per substrain, (by receiving group), has been updated (based on current contagion from source groups).

                // Update effective contagion (force of infection):
                auto& refForceOfInfectionForAntigenAndGroupBySubstrain = refForceOfInfectionForAntigenByGroupAndSubstrain[iGroup];
                // We are going to set current force of infection from current accumulated contagion, clear it out.
                refForceOfInfectionForAntigenAndGroupBySubstrain.clear();

                float population = (normalizeByTotalPopulation ? populationSize : populationSizeByGroup[iGroup]);
                float normalization = ((population == 0.0f) ? 0.0f : 1.0f/population);

                for (const auto& entry : refCurrentContagionForAntigenAndDestinationGroupBySubstrain)
                {
                    auto substrain = entry.first;
                    auto contagion = entry.second;
                    refForceOfInfectionForAntigenAndGroupBySubstrain[substrain] = contagion * infectivityMultiplier * normalization;
                }

                // Force of infection, per substrain, for each group (current contagion * correction * normalization) is up to date.
            }
        }

        for (size_t iAntigen = 0; iAntigen < antigenCount; ++iAntigen)
        {
            size_t groupCount = getGroupCount();
            for (size_t iGroup = 0; iGroup < groupCount; ++iGroup)
            {
                LOG_VALID_F( "(%s) Current contagion [dest] (antigen = %d, route = 0, group = %d) = %f\n", tag.c_str(), iAntigen, iGroup, currentContagionByAntigenAndDestinationGroup[iAntigen][iGroup] );
                for (const auto& entry : currentContagionByAntigenDestinationGroupAndSubstrain[iAntigen][iGroup])
                {
                    LOG_VALID_F( "(%s) Current contagion [dest] (antigen = %d, route = 0, group = %d, substrain = %d) = %f\n", tag.c_str(), iAntigen, iGroup, entry.first, entry.second );
                }
                LOG_VALID_F( "(%s) Force of infection (antigen = %d, route = 0, group = %d) = %f\n", tag.c_str(), iAntigen, iGroup, forceOfInfectionByAntigenAndGroup[iAntigen][iGroup] );
                for (const auto& entry : forceOfInfectionByAntigenGroupAndSubstrain[iAntigen][iGroup])
                {
                    LOG_VALID_F( "(%s) Force of infection (antigen = %d, route = 0, group = %d, substrain = %d) = %f\n", tag.c_str(), iAntigen, iGroup, entry.first, entry.second );
                }
            }
        }

        LOG_VALID_F( "(%s) Exit %s\n", tag.c_str(), __FUNCTION__ );
    }

    float StrainAwareTransmissionGroups::GetTotalContagion( void )
    {
        float contagion = 0.0f;

        for (auto& forceOfInfectionForAntigenByGroup : forceOfInfectionByAntigenAndGroup)
        {
            for (float forceOfInfectionForAntigenAndGroup : forceOfInfectionForAntigenByGroup)
            {
                contagion += forceOfInfectionForAntigenAndGroup;
            }
        }

        return contagion;
    }

    float StrainAwareTransmissionGroups::GetTotalContagionForGroup( TransmissionGroupMembership_t membership )
    {
        float contagion = 0.0f;

        for (auto& forceOfInfectionForAntigenByGroup : forceOfInfectionByAntigenAndGroup)
        {
            float forceOfInfection = forceOfInfectionForAntigenByGroup[ membership.group ];
            contagion += forceOfInfection;
        }

        return contagion;
    }

// NOTYET    float StrainAwareTransmissionGroups::GetTotalContagionForProperties( const IPKeyValueContainer& property_value )
// NOTYET    {
// NOTYET        TransmissionGroupMembership_t txGroups;
// NOTYET        tProperties properties;
// NOTYET        // Convert key:values in IPKeyValueContainer to tProperties
// NOTYET        GetGroupMembershipForProperties( routeIndexToNameMap, properties, &txGroups );
// NOTYET
// NOTYET        return GetTotalContagionForGroup( txGroups );
// NOTYET    }

    BEGIN_QUERY_INTERFACE_BODY(StrainAwareTransmissionGroups::SubstrainPopulationImpl)
    END_QUERY_INTERFACE_BODY(StrainAwareTransmissionGroups::SubstrainPopulationImpl)

    StrainAwareTransmissionGroups::SubstrainPopulationImpl::SubstrainPopulationImpl(
        RANDOMBASE* prng,
	int _antigenId,
	float _quantity,
	const SubstrainMap_t& _substrainDistribution
    )
        : pRNG( prng )
        , antigenId(_antigenId)
        , contagionQuantity(_quantity)
        , substrainDistribution(_substrainDistribution)
    {
    }

    AntigenId StrainAwareTransmissionGroups::SubstrainPopulationImpl::GetAntigenID( void ) const
    {
        return AntigenId(antigenId);
    }

    // This function is stupid because only needed so I can make IStrainIdentity a base class of IContagionPopulation
    AntigenId StrainAwareTransmissionGroups::SubstrainPopulationImpl::GetGeneticID( void ) const
    {
        // Never valid code path, have to implement this method due to interface.
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Not valid for SubstrainPopulationImpl" );
    }

    float StrainAwareTransmissionGroups::SubstrainPopulationImpl::GetTotalContagion( void ) const
    {
        return contagionQuantity;
    }

    void StrainAwareTransmissionGroups::SubstrainPopulationImpl::ResolveInfectingStrain( IStrainIdentity* strainId ) const
    {
        LOG_VALID_F( "%s\n", __FUNCTION__ );
        float totalRawContagion = 0.0f;
        for (auto& entry : substrainDistribution)
        {
            totalRawContagion += entry.second;
        }

        if (totalRawContagion == 0.0f) {
            LOG_WARN_F( "Found no raw contagion for antigen=%d (%f total contagion)\n", antigenId, contagionQuantity);
        }

        float rand = pRNG->e();
        float target = totalRawContagion * rand;
        float contagionSeen = 0.0f;
        int substrainId = 0;

        strainId->SetAntigenID(antigenId);

        for (auto& entry : substrainDistribution)
        {
            float contagion = entry.second;
            if (contagion > 0.0f)
            {
                substrainId = entry.first;
                contagionSeen += contagion;
                if (contagionSeen >= target)
                {
                    LOG_DEBUG_F( "Selected strain id %d\n", substrainId );
                    strainId->SetGeneticID(substrainId); // ????
                    return;
                }
            }
        }

        LOG_WARN_F( "Ran off the end of the distribution (rounding error?). Using last valid sub-strain we saw: %d\n", substrainId );
        strainId->SetGeneticID(substrainId);
    }
}
