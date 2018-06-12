/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "StrainAwareTransmissionGroups.h"
#include "Exceptions.h"

// These includes are required to bring in randgen
#include "Environment.h"
#include "Contexts.h"
#include "RANDOM.h"

#include "Log.h"
#include "Debug.h"
#include "StrainIdentity.h"
#include "SimulationConfig.h"

SETUP_LOGGING( "StrainAwareTransmissionGroups" )

static map< unsigned int, Kernel::TransmissionRoute::Enum > routeIndex2EnumMap;

namespace Kernel
{
    StrainAwareTransmissionGroups::StrainAwareTransmissionGroups()
        : antigenCount(0)
        , substrainCount(0)
        , routeCount(0)
        , antigenWasShed()
        , substrainWasShed()
        , newInfectivityByAntigenRouteGroup()
        , sumInfectivityByAntigenRouteGroup()
        , forceOfInfectionByAntigenRouteGroup()
        , newInfectivityByAntigenRouteGroupSubstrain()
        , sumInfectivityByAntigenRouteGroupSubstrain()
    {
    }

    void StrainAwareTransmissionGroups::Build(const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, int numberOfStrains, int numberOfSubstrains)
    {
        //CheckForValidStrainListSize(strains);
        for (auto& entry : contagionDecayRatesByRoute)
        {
            auto& routeName = entry.first;
            AddRoute( routeName );
        }
        routeCount = getRouteCount();

        BuildRouteScalingMatrices(routeCount);
        StoreRouteDecayValues(routeCount, contagionDecayRatesByRoute);
        AllocateAccumulators(routeCount, numberOfStrains, numberOfSubstrains);

        LOG_DEBUG_F("RouteCount=%d\n",routeCount);
        for (int i=0; i<routeCount; i++)
        {
            LOG_DEBUG_F("For Route %d, Group size = %d\n", i, getGroupCountForRoute(i));
        }
        LOG_DEBUG_F("Built groups with %d strains and %d substrains.\n", numberOfStrains, numberOfSubstrains);
        if(routeIndex2EnumMap.size() == 0 )
        {
            routeIndex2EnumMap.insert( std::make_pair( 0, Kernel::TransmissionRoute::TRANSMISSIONROUTE_CONTACT ) );
            routeIndex2EnumMap.insert( std::make_pair( 1, Kernel::TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL ) );
        }
    }

    void StrainAwareTransmissionGroups::AllocateAccumulators( int routeCount, int numberOfStrains, int numberOfSubstrains )
    {
        LOG_VALID_F( "AllocateAccumulators called with routeCount = %d.\n", routeCount );
        antigenCount = numberOfStrains;
        substrainCount = numberOfSubstrains;

        antigenWasShed.resize(antigenCount);
        vector<bool> allFalse(substrainCount);
        substrainWasShed.resize(antigenCount);

        newInfectivityByAntigenRouteGroup.resize(antigenCount);
        sumInfectivityByAntigenRouteGroup.resize(antigenCount);
        forceOfInfectionByAntigenRouteGroup.resize(antigenCount);
        newInfectivityByAntigenRouteGroupSubstrain.resize(antigenCount);
        sumInfectivityByAntigenRouteGroupSubstrain.resize(antigenCount);
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            newInfectivityByAntigenRouteGroup[iAntigen].resize(routeCount);
            sumInfectivityByAntigenRouteGroup[iAntigen].resize(routeCount);
            forceOfInfectionByAntigenRouteGroup[iAntigen].resize(routeCount);
            newInfectivityByAntigenRouteGroupSubstrain[iAntigen].resize(routeCount);
            sumInfectivityByAntigenRouteGroupSubstrain[iAntigen].resize(routeCount);
            for (int iRoute = 0; iRoute < routeCount; iRoute++)
            {
                int groupCount = getGroupCountForRoute(iRoute);
                newInfectivityByAntigenRouteGroup[iAntigen][iRoute].resize(groupCount);
                sumInfectivityByAntigenRouteGroup[iAntigen][iRoute].resize(groupCount);
                forceOfInfectionByAntigenRouteGroup[iAntigen][iRoute].resize(groupCount);
                newInfectivityByAntigenRouteGroupSubstrain[iAntigen][iRoute].resize(groupCount);
                sumInfectivityByAntigenRouteGroupSubstrain[iAntigen][iRoute].resize(groupCount);
            }
        }

        populationSizeByRoute.resize(routeCount);
    }

    void StrainAwareTransmissionGroups::DepositContagion(const IStrainIdentity& strain, float amount, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        int antigenIndex   = strain.GetAntigenID();
        int substrainIndex = strain.GetGeneticID();
        LOG_DEBUG_F("antigenIndex = %d, substrainIndex = %d\n", antigenIndex, substrainIndex);

        antigenWasShed[antigenIndex]                   = true;
        substrainWasShed[antigenIndex].insert(substrainIndex);

        for (const auto& entry : (*transmissionGroupMembership))
        {
            RouteIndex routeIndex = entry.first;
            GroupIndex groupIndex = entry.second;
            newInfectivityByAntigenRouteGroup[antigenIndex][routeIndex][groupIndex] += amount;
            //release_assert( newInfectivityByAntigenRouteGroupSubstrain[antigenIndex].size() >= routeIndex+1 );
            //release_assert( newInfectivityByAntigenRouteGroupSubstrain[antigenIndex][routeIndex].size() >= groupIndex+1 );
            //release_assert( newInfectivityByAntigenRouteGroupSubstrain[antigenIndex][routeIndex][groupIndex].size() >= substrainIndex+1 );
            newInfectivityByAntigenRouteGroupSubstrain[antigenIndex][routeIndex][groupIndex][substrainIndex] += amount;

            if (amount > 0)
            {
                LOG_DEBUG_F("DepositContagion: sheddingBySubstrain[antigenIndex=%d][routeIndex=%d][groupIndex=%d][substrainIndex=%d] = %f (cum=%f)\n", antigenIndex, routeIndex, groupIndex, substrainIndex, amount, newInfectivityByAntigenRouteGroupSubstrain[antigenIndex][routeIndex][groupIndex][substrainIndex]);
            }
        }
    }

    void StrainAwareTransmissionGroups::ExposeToContagion(IInfectable* candidate, const TransmissionGroupMembership_t* transmissionGroupMembership, float deltaTee) const
    {
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            float forceOfInfection = 0.0f;
            const vector<ContagionAccumulator_t>& forceOfInfectionForRouteAndGroup = forceOfInfectionByAntigenRouteGroup[iAntigen];
            int routeIndex;
            int groupIndex;
            vector<const SubstrainMap_t*> substrainDistributions;

            for (const auto& entry : (*transmissionGroupMembership))
            {
                routeIndex  = entry.first;
                groupIndex  = entry.second;
                forceOfInfection = forceOfInfectionForRouteAndGroup[routeIndex][groupIndex];
                substrainDistributions.push_back(&sumInfectivityByAntigenRouteGroupSubstrain[iAntigen][routeIndex][groupIndex]);

                if ((forceOfInfection > 0) && (candidate != nullptr))
                {
                    LOG_DEBUG_F("ExposureToContagion: [Antigen:%d] Route:%d, Group:%d, exposure qty = %f\n", iAntigen, routeIndex, groupIndex, forceOfInfection );
                    SubstrainPopulationImpl contagionPopulation(iAntigen, forceOfInfection, substrainDistributions);
                    // need a map from route index to enum
                    auto txroute = routeIndex2EnumMap.at( routeIndex );
                    candidate->Expose((IContagionPopulation*)&contagionPopulation, deltaTee, txroute );
                }
                substrainDistributions.clear();
            }
        }
    }

	// Primarily this function returns contagion for the group passed, but can pass nullptr to get total contagion across
    // all groups -- used for exposure skipping and originally implemented by getting contagion for antigen=0, route=0, group=0.
    // Skipping can't be used with HINT (so group==1). Shouldn't be used with multistrain either. This seems like it isn't correct
    // for typhoid (multiroute).
    float StrainAwareTransmissionGroups::GetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        if( transmissionGroupMembership == nullptr )
        {
#if 1 
            float groupInfectionRate = forceOfInfectionByAntigenRouteGroup[0][0][0]; // TBD: 0s not going to work in all cases!
            ContagionPopulationImpl contagionPopulation(groupInfectionRate);
            return ((IContagionPopulation*)&contagionPopulation)->GetTotalContagion();
#endif
            /* Below is proposed implementation from @clorton */
#if 0
			float total = 0.0f;
			// Iterate over antigens
			for (RouteGroupSubstrainMap_t& infectivityForAntigen : sumInfectivityByAntigenRouteGroup)
			{
				// For this antigen, iterate over routes
				for (vector<SubstrainMap_t>& infectivityForRoute : infectivityForAntigen)
				{
					// For this antigen and route, iterate over groups
					for (SubstrainMap_t& substrainsForGroup : infectivityForRoute)
					{
						// For this antigen, route, and group, iterate over substrains
						for (float entry : substrainsForGroup) // TBD: this is still a pair
						{
							total += entry;
						}
					}
				}
			}
			return total;
#endif
        }

        float totalInfectivity = 0;
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            for (const auto& entry : (*transmissionGroupMembership))
            {
                RouteIndex routeIndex = entry.first;
                GroupIndex groupIndex = entry.second;
                float sumInfectivity = forceOfInfectionByAntigenRouteGroup[iAntigen][routeIndex][groupIndex];
                if (sumInfectivity >0)
                {
                    totalInfectivity += sumInfectivity;
                }
                LOG_DEBUG_F("GetTotalContagion: [Antigen:%d] Route:%d, Group:%d, totalContagion = %f\n", iAntigen, routeIndex, groupIndex, sumInfectivity);
            }
        }

        return totalInfectivity;
    }

    void StrainAwareTransmissionGroups::CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        //by antigen total
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            for (const auto& entry : (*transmissionGroupMembership))
            {
                int routeIndex = entry.first;
                int groupIndex = entry.second;
                LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d] Route:%d, Group:%d, ContagionBefore = %f, infectivityCorrection = %f\n", iAntigen, routeIndex, groupIndex, newInfectivityByAntigenRouteGroup[iAntigen][routeIndex][groupIndex], infectivityCorrection);
                newInfectivityByAntigenRouteGroup[iAntigen][routeIndex][groupIndex] *= infectivityCorrection;
                LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d] Route:%d, Group:%d, ContagionAfter = %f\n", iAntigen, routeIndex, groupIndex, newInfectivityByAntigenRouteGroup[iAntigen][routeIndex][groupIndex]);
            }
        }

        //by substrain
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            RouteGroupSubstrainMap_t& shedAntigen = newInfectivityByAntigenRouteGroupSubstrain[iAntigen];
            for (const auto& membership : *transmissionGroupMembership)
            {
                int routeIndex = membership.first;
                int groupIndex = membership.second;

                for (auto& entry : shedAntigen[routeIndex][groupIndex])
                {
                    unsigned int iSubstrain = entry.first;
                    LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d][Route:%d][Group:%d][Substrain:%d], ContagionBefore = %f, infectivityCorrection = %f\n", iAntigen, routeIndex, groupIndex, iSubstrain, shedAntigen[routeIndex][groupIndex][iSubstrain], infectivityCorrection);
                    entry.second *= infectivityCorrection;
                    LOG_DEBUG_F("CorrectInfectivityByGroup: [Antigen:%d][Route:%d][Group:%d][Substrain:%d], ContagionAfter  = %f\n", iAntigen, routeIndex, groupIndex, iSubstrain, shedAntigen[routeIndex][groupIndex][iSubstrain]);
                }
            }
        }
    }

    void StrainAwareTransmissionGroups::EndUpdate(float infectivityCorrection)
    {
        // --===#### Take care of antigen totals ####===--
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            for (int iRoute = 0; iRoute < routeCount; iRoute++)
            {
                // Decay current contagion (regardless of current shedding)
                ContagionAccumulator_t& currentAntigen = sumInfectivityByAntigenRouteGroup[iAntigen][iRoute];
                float decayRate = 1.0f - contagionDecayRates[iRoute];
                LOG_DEBUG_F( "Applying decay rate of %f on route %d to antigen accumulation of %f\n", decayRate, iRoute, currentAntigen[0] );
                VectorScalarMultiplyInPlace(currentAntigen, decayRate);

                int groupCount = getGroupCountForRoute(iRoute);

                if (antigenWasShed[iAntigen])
                {
                    const ScalingMatrix_t& scalingMatrix = scalingMatrices[iRoute];
                    const ContagionAccumulator_t& shedAntigen = newInfectivityByAntigenRouteGroup[iAntigen][iRoute];
                    for (int iSink = 0; iSink < groupCount; iSink++)
                    {
                        const MatrixRow_t& betaVector = scalingMatrix[iSink];
                        float deposit = VectorDotProduct(shedAntigen, betaVector);
                        deposit *= infectivityCorrection;
                        LOG_DEBUG_F("Adding %f to %f contagion [antigen:%d,route:%d,group:%d]\n", deposit, currentAntigen[iSink], iAntigen, iRoute, iSink);
                        currentAntigen[iSink] += deposit;
                    }

                    // Reset for next cycle.
                    memset((void*)shedAntigen.data(), 0, shedAntigen.size() * sizeof(float));
                }

                float populationForRoute = populationSizeByRoute[iRoute];
                vector<float>& forceOfInfectionForAntigenAndRoute = forceOfInfectionByAntigenRouteGroup[iAntigen][iRoute];
                if (populationForRoute > 0)
                {
                    LOG_DEBUG_F("Contagion for [antigen:%d,route:%d] scaled by %f\n", iAntigen, iRoute, populationForRoute);
                    memcpy(forceOfInfectionForAntigenAndRoute.data(), currentAntigen.data(), groupCount * sizeof(float));
                    VectorScalarMultiplyInPlace(forceOfInfectionForAntigenAndRoute, 1.0f/populationForRoute);
                }
                else
                {
                    memset(forceOfInfectionForAntigenAndRoute.data(), 0, groupCount * sizeof(float));
                }

                for( auto element : forceOfInfectionForAntigenAndRoute )
                {
                    LOG_VALID_F( "foi for route %d (antigen=%d)= %f\n", iRoute, iAntigen, element );
                }
            }
        }

        // Take care of substrain decay
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            for (int iRoute = 0; iRoute < routeCount; iRoute++)
            {
                float remainingFraction = 1.0f - contagionDecayRates[iRoute];
                int groupCount = getGroupCountForRoute(iRoute);
                for (int iSink = 0; iSink < groupCount; iSink++)
                {
                    SubstrainMap_t& currentContagion = sumInfectivityByAntigenRouteGroupSubstrain[iAntigen][iRoute][iSink];
                    if(remainingFraction == 0)
                    {
                        currentContagion.clear();
                    }
                    else
                    {
                        for (auto& entry : currentContagion)
                        {
                            entry.second *= remainingFraction;
                        }
                    }
                }
            }
        }

        // Take care of substrain shedding and distribution
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            if (antigenWasShed[iAntigen])
            {
                RouteGroupSubstrainMap_t& newInfectivityByRouteGroupSubstrain = newInfectivityByAntigenRouteGroupSubstrain[iAntigen];
                RouteGroupSubstrainMap_t& sumInfectivity = sumInfectivityByAntigenRouteGroupSubstrain[iAntigen];

                auto& substrainShedding = substrainWasShed[iAntigen];
                for(auto iSubstrain : substrainShedding)
                {
                    ContagionAccumulator_t newInfectivityForSubstrainByGroup;

                    for (int iRoute = 0; iRoute < routeCount; iRoute++)
                    {
                        vector<SubstrainMap_t>& newInfectivityByGroupSubstrain = newInfectivityByRouteGroupSubstrain[iRoute];
                        int groupCount = getGroupCountForRoute(iRoute);
                        newInfectivityForSubstrainByGroup.resize(groupCount);
                        for (int iGroup = 0; iGroup < groupCount; iGroup++)
                        {
                            SubstrainMap_t::iterator it = newInfectivityByGroupSubstrain[iGroup].find(iSubstrain);
                            if(it != newInfectivityByGroupSubstrain[iGroup].end())
                            {
                                newInfectivityForSubstrainByGroup[iGroup] = newInfectivityByGroupSubstrain[iGroup][iSubstrain];
                                newInfectivityByGroupSubstrain[iGroup].erase(it);
                            }
                        }

                        vector<SubstrainMap_t>& sumSubstrainInfectivity = sumInfectivity[iRoute];

                        const ScalingMatrix_t& scalingMatrix = scalingMatrices[iRoute];
                        for (int iSink = 0; iSink < groupCount; iSink++)
                        {
                            const MatrixRow_t& contactScaling = scalingMatrix[iSink];
                            float deposit = VectorDotProduct(newInfectivityForSubstrainByGroup, contactScaling);
                            deposit *= infectivityCorrection;
                            if (deposit > 0.0f)
                            {
                                sumSubstrainInfectivity[iSink][iSubstrain] += deposit;
                                LOG_DEBUG_F( "exposureBySubstrain for substrain %d now = %f\n", iSubstrain, sumInfectivityByAntigenRouteGroupSubstrain[iAntigen][iRoute][iSink][iSubstrain] );
                            }
                        }
                    }
                }

                // Reset for next cycle.
                substrainShedding.clear();
                antigenWasShed[iAntigen] = false;
            }
        }
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            for (int iRoute = 0; iRoute < routeCount; iRoute++)
            {
                vector<float>& forceOfInfectionForAntigenAndRoute = forceOfInfectionByAntigenRouteGroup[iAntigen][iRoute];
                for( auto element : forceOfInfectionForAntigenAndRoute )
                {
                    LOG_VALID_F( "foi for route %d (antigen=%d)= %f\n", iRoute, iAntigen, element );
                }
            }
        }
    }

    BEGIN_QUERY_INTERFACE_BODY(StrainAwareTransmissionGroups::SubstrainPopulationImpl)
    END_QUERY_INTERFACE_BODY(StrainAwareTransmissionGroups::SubstrainPopulationImpl)

    StrainAwareTransmissionGroups::SubstrainPopulationImpl::SubstrainPopulationImpl(int _antigenId, float _quantity, const vector<const SubstrainMap_t*>& _substrainDistributions)
    : antigenId(_antigenId)
    , contagionQuantity(_quantity)
    , substrainDistributions(_substrainDistributions)
    {
        //release_assert( antigenId );
    }

    AntigenId StrainAwareTransmissionGroups::SubstrainPopulationImpl::GetAntigenID( void ) const
    {
        return (AntigenId)antigenId;
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
        int routeCount = substrainDistributions.size();
        for (int iRoute = 0; iRoute < routeCount; iRoute++)
        {
            const SubstrainMap_t& distribution = *(substrainDistributions[iRoute]);
            for (auto& entry : distribution)
            {
                totalRawContagion += entry.second;
            }
        }

        if (totalRawContagion == 0.0f) {
            LOG_WARN_F( "Found no raw contagion for antigen=%d (%f total contagion)\n", antigenId, contagionQuantity);
        }

        float rand = randgen->e();
        float target = totalRawContagion * rand;
        float contagionSeen = 0.0f;
        int substrainId = 0;

        strainId->SetAntigenID(antigenId);

        for (int iRoute = 0; iRoute < routeCount; iRoute++)
        {
            const SubstrainMap_t& distribution = *(substrainDistributions[iRoute]);
            for (auto& entry : distribution)
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
        }

        LOG_WARN_F( "Ran off the end of the distribution (rounding error?). Using last valid sub-strain we saw: %d\n", substrainId );
        strainId->SetGeneticID(substrainId);
    }
}
