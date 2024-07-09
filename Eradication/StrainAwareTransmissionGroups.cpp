
#include "stdafx.h"

#include "StrainAwareTransmissionGroups.h"
#include "Exceptions.h"
#include "RANDOM.h"
#include "Log.h"
#include "Debug.h"
#include "IStrainIdentity.h"
#include "VectorMath.h"
#include "TransmissionGroupsUtils.h"
#include "GeneticProbability.h" // needed for template instantiation
#include <numeric>

SETUP_LOGGING( "StrainAwareTransmissionGroups" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- StrainAwareTransmissionGroupsTemplate
    // ------------------------------------------------------------------------

    template<typename T>
    StrainAwareTransmissionGroupsTemplate<T>::StrainAwareTransmissionGroupsTemplate( RANDOMBASE* prng )
        : pRNG( prng )
        , propertyNameToMatrixMap()
        , propertyValueToIndexMap()
        , propertyToValuesMap()
        , scalingMatrix()
        , contagionDecayRate( 1.0f )
        , populationSize( 0.0f )
        , populationSizeByGroup()
        , antigenCount(0)
        , substrainCount(0)
        , normalizeByTotalPopulation(true)
        , newlyDepositedContagionByAntigenAndGroup()
        , currentContagionByAntigenAndSourceGroup()
        , currentContagionByAntigenAndDestinationGroup()
        , forceOfInfectionByAntigenAndGroup()
        , newContagionByAntigenGroupAndSubstrain()
        , currentContagionByAntigenSourceGroupAndSubstrain()
        , currentContagionByAntigenDestinationGroupAndSubstrain()
        , forceOfInfectionByAntigenGroupAndSubstrain()
        , tag("contact")
        , totalContagion(0)
    {
    }

    template<typename T>
    StrainAwareTransmissionGroupsTemplate<T>::~StrainAwareTransmissionGroupsTemplate()
    {
    }

    template<typename T>
    int StrainAwareTransmissionGroupsTemplate<T>::getGroupCount()
    {
        return scalingMatrix.size();
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::UseTotalPopulationForNormalization()
    { 
        normalizeByTotalPopulation = true;
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::UseGroupPopulationForNormalization()
    { 
        normalizeByTotalPopulation = false;
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::SetTag( const std::string& tg )
    {
        tag = tg;
    }

    template<typename T>
    const std::string& StrainAwareTransmissionGroupsTemplate<T>::GetTag( void ) const
    {
        return tag;
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::checkForDuplicatePropertyName( const string& property ) const
    {
        if (propertyNameToMatrixMap.find(property) != propertyNameToMatrixMap.end())
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Duplicated property name.");
        }
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::addScalingMatrixToPropertyToMatrixMap( const string& property, const ScalingMatrix_t& scalingMatrix )
    {
        propertyNameToMatrixMap[property] = scalingMatrix;
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::addPropertyValuesToValueToIndexMap( const string& propertyName, const PropertyValueList_t& valueSet, int currentMatrixSize )
    {
        ValueToIndexMap_t valueToIndexMap;
        int valueIndex = 0;
        for (auto& value : valueSet)
        {
            valueToIndexMap[value] = valueIndex * currentMatrixSize;
            valueIndex++;
        }

        propertyValueToIndexMap[propertyName] = valueToIndexMap;
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::getGroupIndicesForProperty( const IPKeyValue& property_value, const PropertyToValuesMap_t& propertyNameToValuesMap, std::vector<size_t>& indices )
    {
        const string& key = property_value.GetKeyAsString();

        indices.clear();
        indices.push_back( 0 );
        // propertyNameToValuesMap is the list of properties to be considered.
        // Iterate over these properties for the following:
        // If the property is the one passed in, use the propertyValueToIndexMap to get the index offset for the value passed in.
        // Else, it is a different property, add indices for each possible value of this property with its offset.
        // E.g. single property (RISK:HIGH|MEDIUM|LOW), property_value is "RISK:LOW":
        //  indices ends up holding a single index, 2, which will fetch the contagion deposited to the RISK:LOW group.
        // E.g. multiple properties  (RISK:HIGH|MEDIUM|LOW) & (GEOGRAPHY:BOTHELL|BELLEVUE), property_value is "GEOGRAPHY:BOTHELL"
        //  indices ends up holding three index values, 0, 1, and 2, which correspond to HIGH-BOTHELL, MEDIUM-BOTHELL, and LOW-BOTHELL.
        for (auto& entry : propertyNameToValuesMap)
        {
            // entry.first == property name
            // entry.second == values list
            if (entry.first == key)
            {
                // For every value in indices, _add_ the offset to the property value
                size_t offset = propertyValueToIndexMap.at(key).at(property_value.GetValueAsString());
                for (auto& index : indices)
                {
                    index += offset;
                }
            }
            else
            {
                std::vector<size_t> temp(indices);
                indices.clear();
                for (auto& value : entry.second)
                {
                    size_t offset = propertyValueToIndexMap.at(entry.first).at(value);
                    for (size_t index : temp)
                    {
                        indices.push_back(index + offset);
                    }
                }
            }
        }

        return;
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::AddProperty( const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix )
    {
        LOG_DEBUG_F( "Adding property %s\n", property.c_str() );
        checkForDuplicatePropertyName(property);
        TransmissionGroupsUtils::checkForValidValueListSize(values);
        TransmissionGroupsUtils::checkForValidScalingMatrixSize(scalingMatrix, values);

        addPropertyValueListToPropertyToValueMap(property, values);
        addScalingMatrixToPropertyToMatrixMap(property, scalingMatrix);
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::addPropertyValueListToPropertyToValueMap( const string& property, const PropertyValueList_t& values )
    {
        propertyToValuesMap[property] = values;
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::buildScalingMatrix( void )
    {
        LOG_DEBUG_F( "%s\n", __FUNCTION__ );

        ScalingMatrix_t cumulativeMatrix;
        TransmissionGroupsUtils::InitializeCumulativeMatrix(cumulativeMatrix);

        // For each property, aggregate the propertyMatrix with the cumulative scaling matrix
        for (const auto& pair : propertyToValuesMap)
        {
            const string& propertyName = pair.first;
            const PropertyValueList_t& valueList = pair.second;
            addPropertyValuesToValueToIndexMap(propertyName, valueList, cumulativeMatrix.size());

            TransmissionGroupsUtils::AggregatePropertyMatrixWithCumulativeMatrix( propertyNameToMatrixMap[propertyName],
                                                                                  cumulativeMatrix );
        }

        scalingMatrix = cumulativeMatrix;
    }

    // EVERYTHING UP TO HERE WAS SETUP. AFTER HERE IS RUNTIME STUFF.

    /* 
     * The goal here is to create and return a map of route indices to group indices. So each route,
     * e.g., environmental and contact, represented by an id, say 1 and 0, will map to property "ids",
     * aka group ids, like 0-10.
     */
    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::GetGroupMembershipForProperties( const IPKeyValueContainer& properties,
                                                                                    TransmissionGroupMembership_t& membershipOut ) const
    {
        // Start at 0. Will, potentially, modify below based on property values and their index offsets.
        membershipOut.group = 0;

        for( const auto& entry : properties )
        {
            const string& propertyName = entry.GetKeyAsString();
            if( propertyNameToMatrixMap.find( propertyName ) != propertyNameToMatrixMap.end() )
            {
                const string& propertyValue = entry.GetValueAsString();
                if( propertyValueToIndexMap.find( propertyName ) != propertyValueToIndexMap.end() &&
                    propertyValueToIndexMap.at( propertyName ).find( propertyValue ) != propertyValueToIndexMap.at( propertyName ).end() )
                {
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

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::UpdatePopulationSize(const TransmissionGroupMembership_t& membership, float size_changes, float mc_weight)
    {
        float delta = size_changes * mc_weight;
        populationSize += delta;
        populationSizeByGroup[membership.group] += delta;
    }

    template<typename T>
    float StrainAwareTransmissionGroupsTemplate<T>::GetPopulationSize( const TransmissionGroupMembership_t& transmissionGroupMembership ) const
    {
        GroupIndex iGroup = transmissionGroupMembership.group;
        float population = (normalizeByTotalPopulation ? populationSize : populationSizeByGroup[iGroup]);
        return population;
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::ClearPopulationSize()
    {
        populationSize = 0.0f;
        std::fill( populationSizeByGroup.begin(), populationSizeByGroup.end(), 0.0f );
    }
        
    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::Build(float contagionDecayRate, int numberOfStrains, int numberOfSubstrains)
    {
        buildScalingMatrix();
        this->contagionDecayRate = contagionDecayRate;
        allocateAccumulators(numberOfStrains, numberOfSubstrains);

        LOG_DEBUG_F("Built %d groups with %d strains and %d substrains.\n", getGroupCount(), numberOfStrains, numberOfSubstrains);
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::allocateAccumulators( NaturalNumber numberOfStrains, NaturalNumber numberOfSubstrains )
    {
        LOG_VALID( "AllocateAccumulators called.\n" );
        antigenCount = numberOfStrains;
        substrainCount = numberOfSubstrains;

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

    template<typename T>
    T StrainAwareTransmissionGroupsTemplate<T>::getTotalContagionInner( void )
    {
        T contagion = 0.0f;

        for (auto& forceOfInfectionForAntigenByGroup : forceOfInfectionByAntigenAndGroup)
        {
            for (T forceOfInfectionForAntigenAndGroup : forceOfInfectionForAntigenByGroup)
            {
                contagion += forceOfInfectionForAntigenAndGroup;
            }
        }

        return contagion;
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::depositContagionInner(const IStrainIdentity& strain, const T& amount, TransmissionGroupMembership_t membership)
    {
        int iAntigen    = strain.GetAntigenID();
        int substrainId = strain.GetGeneticID();

        if ( iAntigen >= antigenCount )
        {
            ostringstream msg;
            msg << "Strain antigen ID (" << iAntigen << ") >= configured number of strains (" << antigenCount << ").\n";
            throw new OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str(), float(iAntigen), float(antigenCount) );
        }

        GroupIndex iGroup = membership.group;
        newlyDepositedContagionByAntigenAndGroup[iAntigen][iGroup] += amount;
        newContagionByAntigenGroupAndSubstrain[iAntigen][iGroup][substrainId] += amount;

        LOG_VALID_F( "(%s) DepositContagion (antigen = %d, route = 0, group = %d [substrain = %d]) increased by %f\n",
                        tag.c_str(),
                        iAntigen,
                        iGroup,
                        substrainId,
                        float(amount) );
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::ExposeToContagion( IInfectable* candidate,
                                                                      TransmissionGroupMembership_t membership,
                                                                      float deltaTee,
                                                                      TransmissionRoute::Enum txRoute )
    {
        release_assert( candidate != nullptr );

        //LOG_DEBUG_F( "ExposeToContagion\n" );
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            T forceOfInfection = 0.0f;
            const ContagionAccumulatorTemplate<T>& forceOfInfectionByGroup = forceOfInfectionByAntigenAndGroup[iAntigen];

            size_t iGroup = membership.group;
            forceOfInfection = forceOfInfectionByGroup[iGroup];

            if( isGreaterThanZero( forceOfInfection ) )
            {
                LOG_DEBUG_F("ExposureToContagion: [Antigen:%d] Route:0, Group:%d, exposure qty = %f\n", iAntigen, iGroup, float(forceOfInfection) );

                exposeCandidate( candidate,
                                 iAntigen,
                                 forceOfInfection,
                                 forceOfInfectionByAntigenGroupAndSubstrain[ iAntigen ][ iGroup ],
                                 deltaTee,
                                 txRoute );
            }
        }
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::CorrectInfectivityByGroup(float infectivityMultiplier, TransmissionGroupMembership_t membership)
    {
        // By antigen (substrains aggregated)
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            int iGroup = membership.group;

            LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d] Route:0, Group:%d, ContagionBefore = %f, infectivityMultiplier = %f\n",
                         iAntigen, iGroup, float(newlyDepositedContagionByAntigenAndGroup[iAntigen][iGroup]), infectivityMultiplier);

            newlyDepositedContagionByAntigenAndGroup[iAntigen][iGroup] *= infectivityMultiplier;

            LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d] Route:0, Group:%d, ContagionAfter = %f\n",
                         iAntigen, iGroup, float(newlyDepositedContagionByAntigenAndGroup[iAntigen][iGroup]));
        }

        // By individual substrain
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            GroupSubstrainMapTemplate<T>& shedAntigen = newContagionByAntigenGroupAndSubstrain[iAntigen];
            int iGroup = membership.group;
            for (auto& entry : shedAntigen[iGroup])
            {
                uint32_t substrainId = entry.first;

                LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d][Route:0][Group:%d][Substrain:%d], ContagionBefore = %f, infectivityMultiplier = %f\n",
                             iAntigen, iGroup, substrainId, float(shedAntigen[iGroup][substrainId]), infectivityMultiplier);

                entry.second *= infectivityMultiplier;

                LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d][Route:0][Group:%d][Substrain:%d], ContagionAfter  = %f\n",
                             iAntigen, iGroup, substrainId, float(shedAntigen[iGroup][substrainId]));
            }
        }
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::EndUpdate( float infectivityMultiplier, float infectivityAddition )
    {
        LOG_VALID_F( "(%s) Enter (%s)\n", tag.c_str(), __FUNCTION__ );

        float additionalContagion = infectivityAddition;

        if ( (infectivityAddition != 0.0f) &&
             ((getGroupCount() > 1) || (antigenCount > 1) || (substrainCount > 1)) )
        {
            LOG_WARN_F( "StrainAwareTransmissionGroupsTemplate::EndUpdate() infectivityAddition != 0 (%f actual), but one or more of # HINT groups (%d), antigen count (%d), or substrain count (%d) is > 1. Using 0 for additional contagion.\n",
                        infectivityAddition, getGroupCount(), antigenCount, substrainCount );
            additionalContagion = 0.0f;
        }

        int groupCount = getGroupCount();
        if (EnvPtr->Log->SimpleLogger::IsLoggingEnabled(Logger::VALIDATION, _module, _log_level_enabled_array))
        {
            for (int iAntigen = 0; iAntigen < antigenCount; ++iAntigen)
            {
                for (int iGroup = 0; iGroup < groupCount; ++iGroup)
                {
                    LOG_VALID_F( "(%s) New contagion (antigen = %d, route = 0, group = %d) = %f\n",
                                 tag.c_str(), iAntigen, iGroup, float(newlyDepositedContagionByAntigenAndGroup[iAntigen][iGroup]) );
                    for (const auto& entry : newContagionByAntigenGroupAndSubstrain[iAntigen][iGroup])
                    {
                        LOG_VALID_F( "(%s) New contagion (antigen = %d, route = 0, group = %d, substrain = %d) = %f\n",
                                     tag.c_str(), iAntigen, iGroup, entry.first, float(entry.second) );
                    }
                    LOG_VALID_F( "(%s) Current contagion [source] (antigen = %d, route = 0, group = %d) = %f\n",
                                 tag.c_str(), iAntigen, iGroup, float(currentContagionByAntigenAndSourceGroup[iAntigen][iGroup]) );
                    for (const auto& entry : currentContagionByAntigenSourceGroupAndSubstrain[iAntigen][iGroup])
                    {
                        LOG_VALID_F( "(%s) Current contagion [source] (antigen = %d, route = 0, group = %d, substrain = %d) = %f\n",
                                     tag.c_str(), iAntigen, iGroup, entry.first, float(entry.second) );
                    }
                }
            }
        }

        // For each antigen...
        for (int iAntigen = 0; iAntigen < antigenCount; ++iAntigen)
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
            if( decayFactor == 0.0 )
            {
                for( auto& entry : refCurrentContagionForAntigenBySourceGroup )
                {
                    // For GeneticProbability, this will replace the elements in the array.
                    // If it has specific species info, this will get rid of it.  We need
                    // to do this so that we can get truly reset and only get the species
                    // specific data of the newlyDepositedContagionXXX
                    entry = T( 0.0f );
                }
            }
            else
            {
                VectorMath::scalarMultiply( refCurrentContagionForAntigenBySourceGroup, decayFactor );
            }
            VectorMath::elementAdd( refCurrentContagionForAntigenBySourceGroup, refNewlyDepositedContagionForAntigenByGroup );
            VectorMath::scalarAdd( refCurrentContagionForAntigenBySourceGroup, additionalContagion );

            // We just added this to the current contagion accumulator. Clear it out.
            for( auto& entry : refNewlyDepositedContagionForAntigenByGroup )
            {
                // For GeneticProbability, this will replace the elements in the array.
                entry = T( 0.0f );
            }

            // Current contagion (by group which deposited it) is up to date and new contagion by group is reset.

            // Update accumulated contagion by destination group:
            for (int iGroup = 0; iGroup < groupCount; ++iGroup)
            {
                const MatrixRow_t& betaVector = scalingMatrix[iGroup];
                T accumulatedContagion = VectorMath::dotProduct( refCurrentContagionForAntigenBySourceGroup, betaVector );
                LOG_VALID_F("(%s) Adding %f to %f contagion [antigen:%d,route:0,group:%d]\n", tag.c_str(), float(accumulatedContagion), 0.0f, iAntigen, iGroup);
                refCurrentContagionForAntigenByDestinationGroup[iGroup] = accumulatedContagion;
            }

            // Current contagion (by receiving group) is up to date (based on current contagion from source groups).

            for (int iGroup = 0; iGroup < groupCount; ++iGroup)
            {
                // Update effective contagion (force of infection) by destination group:
                float population = (normalizeByTotalPopulation ? populationSize : populationSizeByGroup[iGroup]);
                float normalization = ((population == 0.0f) ? 0.0f : 1.0f/population);
                LOG_VALID_F( "(%s) Normalization (%s) for group %d is %f based on population %f\n",
                             tag.c_str(), (normalizeByTotalPopulation ? "total population" : "group population"), iGroup, normalization, population );
                LOG_VALID_F( "(%s) Contagion for [antigen:%d,route:0] population scaled by %f\n", tag.c_str(), iAntigen, population);

                refForceOfInfectionForAntigenByGroup[iGroup] = refCurrentContagionForAntigenByDestinationGroup[iGroup]
                                                                * infectivityMultiplier
                                                                * normalization;
            }

            // Force of infection for each group (current contagion * correction * normalization) is up to date.

            for (int iGroup = 0; iGroup < groupCount; ++iGroup)
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

            for (int iGroup = 0; iGroup < groupCount; ++iGroup)
            {
                auto& refCurrentContagionForAntigenAndDestinationGroupBySubstrain = refCurrentContagionForAntigenByDestinationGroupAndSubstrain[iGroup];

                const MatrixRow_t& betaVector = scalingMatrix[iGroup];

                // We are going to transfer accumulated contagion to this structure, clear it out.
                refCurrentContagionForAntigenAndDestinationGroupBySubstrain.clear();

                // Update accumulated contagion, by substrain, indexed by destination group:
                for (int srcGroup = 0; srcGroup < groupCount; ++srcGroup)
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

        totalContagion = getTotalContagionInner();

        if (EnvPtr->Log->SimpleLogger::IsLoggingEnabled(Logger::VALIDATION, _module, _log_level_enabled_array))
        {
            for (int iAntigen = 0; iAntigen < antigenCount; ++iAntigen)
            {
                for (int iGroup = 0; iGroup < groupCount; ++iGroup)
                {
                    LOG_VALID_F( "(%s) Current contagion [dest] (antigen = %d, route = 0, group = %d) = %f\n",
                                 tag.c_str(), iAntigen, iGroup, float(currentContagionByAntigenAndDestinationGroup[iAntigen][iGroup]) );
                    for (const auto& entry : currentContagionByAntigenDestinationGroupAndSubstrain[iAntigen][iGroup])
                    {
                        LOG_VALID_F( "(%s) Current contagion [dest] (antigen = %d, route = 0, group = %d, substrain = %d) = %f\n",
                                     tag.c_str(), iAntigen, iGroup, entry.first, float(entry.second) );
                    }
                    LOG_VALID_F( "(%s) Force of infection (antigen = %d, route = 0, group = %d) = %f\n",
                                 tag.c_str(), iAntigen, iGroup, float(forceOfInfectionByAntigenAndGroup[iAntigen][iGroup]) );
                    for (const auto& entry : forceOfInfectionByAntigenGroupAndSubstrain[iAntigen][iGroup])
                    {
                        LOG_VALID_F( "(%s) Force of infection (antigen = %d, route = 0, group = %d, substrain = %d) = %f\n",
                                     tag.c_str(), iAntigen, iGroup, entry.first, float(entry.second) );
                    }
                }
            }

            LOG_VALID_F( "(%s) Exit %s\n", tag.c_str(), __FUNCTION__ );
        }
    }

    template<typename T>
    void StrainAwareTransmissionGroupsTemplate<T>::ClearStrain( const IStrainIdentity* pStrain,
                                                                const TransmissionGroupMembership_t& membership )
    {
        // ------------------------------------------------------------------------------------------------------
        // --- The idea here is to remove the strain so it can't be selected again by ResolveInfectingStrain().
        // --- This was needed in the Malaria Simple Co-Transmission work where a "strain" was a vector.
        // --- !!! We don't remove the amount from forceOfInfectionByAntigenAndGroup because we want
        // --- !!! IContagionPopulation::GetTotalContagion() to return the same value during the timestep.
        // --- !!! If we reduce it, the people later in the processing list of humans will have less of a chance
        // --- !!! of being exposed.  That is, people who get processed first have a higher chance of being
        // --- !!! exposed just because they are first.
        // ------------------------------------------------------------------------------------------------------
        uint32_t i_antigen = pStrain->GetAntigenID();
        uint32_t i_strain = pStrain->GetGeneticID();
        forceOfInfectionByAntigenGroupAndSubstrain[ i_antigen ][ membership.group ].erase( i_strain );
    }

    // -----------------------------------------------------------------------------
    // --- This instatiates the template for the version of the tempalte
    // --- with GeneticProbability.  It eliminates the need to put these template
    // --- methods into the header file. (Template instantiation is done to ensure
    // --- that the methods are created and we don't end up with linker issues.)
    // -----------------------------------------------------------------------------
    template class StrainAwareTransmissionGroupsTemplate<GeneticProbability>;

    // ------------------------------------------------------------------------
    // --- StrainAwareTransmissionGroups
    // ------------------------------------------------------------------------

    StrainAwareTransmissionGroups::StrainAwareTransmissionGroups( RANDOMBASE* prng )
        : StrainAwareTransmissionGroupsTemplate( prng )
    {
    }

    StrainAwareTransmissionGroups::~StrainAwareTransmissionGroups()
    {
    }

    void StrainAwareTransmissionGroups::exposeCandidate( IInfectable* candidate, 
                                                         int iAntigen,
                                                         const float& forceOfInfection,
                                                         const SubstrainMapTemplate<float>& substrainMap,
                                                         float dt,
                                                         TransmissionRoute::Enum txRoute )
    {
        ContagionPopulationSubstrain contagionPopulation( pRNG, iAntigen, forceOfInfection, substrainMap );
        candidate->Expose( (IContagionPopulation*)&contagionPopulation, dt, txRoute );
    }

    void StrainAwareTransmissionGroups::DepositContagion( const IStrainIdentity& strain,
                                                          float amount,
                                                          TransmissionGroupMembership_t transmissionGroupMembership )
    {
        depositContagionInner( strain, amount, transmissionGroupMembership );
    }

    float StrainAwareTransmissionGroups::GetTotalContagion( void )
    {
        return getTotalContagionInner();
    }

    float StrainAwareTransmissionGroups::GetContagionByProperty(const IPKeyValue& property_value)
    {
        std::vector<size_t> indices;
        getGroupIndicesForProperty( property_value, propertyToValuesMap, indices );
        float total = 0.0f;
        for (int iAntigen = 0; iAntigen < antigenCount; ++iAntigen)
        {
            const auto& contagion = forceOfInfectionByAntigenAndGroup[iAntigen];
            total += std::accumulate(indices.begin(), indices.end(), 0.0f, [&](float init, size_t index) { return init + contagion[index]; });
        }

        return total;
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

    bool StrainAwareTransmissionGroups::isGreaterThanZero( const float& forceOfInfection ) const
    {
        return (forceOfInfection > 0.0f);
    }

    // ------------------------------------------------------------------------
    // --- ContagionPopulationSubstrain
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(ContagionPopulationSubstrain)
    END_QUERY_INTERFACE_BODY(ContagionPopulationSubstrain)

    ContagionPopulationSubstrain::ContagionPopulationSubstrain( RANDOMBASE* prng,
                                                                int _antigenId,
                                                                float _quantity,
                                                                const SubstrainMap_t& _substrainDistribution )
        : ContagionPopulationSimple( _antigenId, _quantity )
        , m_pRNG( prng )
        , m_rSubstrainDistribution(_substrainDistribution)
    {
    }

    ContagionPopulationSubstrain::~ContagionPopulationSubstrain()
    {
    }

    bool ContagionPopulationSubstrain::ResolveInfectingStrain( IStrainIdentity* strainId ) const
    {
        LOG_VALID_F( "%s\n", __FUNCTION__ );

        strainId->SetAntigenID( m_AntigenID );
        strainId->SetGeneticID( 0 );

        float totalRawContagion = 0.0f;
        for (auto& entry : m_rSubstrainDistribution)
        {
            totalRawContagion += entry.second;
        }

        if (totalRawContagion == 0.0f)
        {
            // ---------------------------------------------------------------------------
            // --- Because Malaria CoTransmission uses ClearStrain() to remove "strains"
            // --- (i.e. vectors and humans).  The actual "raw" contagion can go away.
            // --- This implies that all of the bites have been distributed.
            // --- NOTE:  This can also happen when Enable_Infectivity_Reservoir is enabled.
            // ---        data can be infectivity can be added to currentContagionByAntigenAndSourceGroup
            // ---        but not currentContagionByAntigenSourceGroupAndSubstrain.  It is
            // ---        the way contagion can enter the TG without going through DepositContagion().
            // ---------------------------------------------------------------------------

            // commenting out because Malaria CoTransmission can have this happen
            //LOG_WARN_F( "Found no raw contagion for antigen=%d (%f total contagion)\n", antigenId, contagionQuantity);

            return false;
        }

        float rand = m_pRNG->e();
        float target = totalRawContagion * rand;
        float contagionSeen = 0.0f;
        int substrainId = 0;

        for (auto& entry : m_rSubstrainDistribution)
        {
            float contagion = entry.second;
            if (contagion > 0.0f)
            {
                substrainId = entry.first;
                contagionSeen += contagion;
                if (contagionSeen >= target)
                {
                    LOG_DEBUG_F( "Selected strain id %d\n", substrainId );
                    strainId->SetGeneticID(substrainId);
                    return true;
                }
            }
        }

        // commenting out because Malaria CoTransmission can have this happen
        //LOG_WARN_F( "Ran off the end of the distribution (rounding error?). Using last valid sub-strain we saw: %d\n", substrainId );
        strainId->SetGeneticID(substrainId);

        return false;
    }
}
