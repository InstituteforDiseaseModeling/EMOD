/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "RelationshipGroups.h"
#include "Relationship.h"
#include "NodeSTI.h"
#include "Debug.h"
#include <algorithm>

#include <ctime>

static const char * _module = "RelationshipGroups";

extern void howlong(
    clock_t before,
    const char * label = "NA"
);

namespace Kernel {

#ifndef DISABLE_STI
    //IContagionPopulation
    //IContagionProbabilities
    BEGIN_QUERY_INTERFACE_BODY(DiscreteContagionPopulation)
        HANDLE_INTERFACE(IContagionPopulation)
        HANDLE_INTERFACE(IContagionProbabilities)
    END_QUERY_INTERFACE_BODY(DiscreteContagionPopulation)

    RelationshipGroups::RelationshipGroups()
    {
    }

    void RelationshipGroups::SetParent(
        INodeSTI * parent
    )
    {
        m_parent = parent;
    }

    void RelationshipGroups::AddPropertyValuesToValueToIndexMap(
        const string& propertyName,
        const PropertyValueList_t& valueSet,
        int currentMatrixSize
    )
    {
        LOG_DEBUG_F( "%s: propertyName = %s\n", __FUNCTION__, propertyName.c_str() );

        ValueToIndexMap_t &valueToIndexMap = propertyValueToIndexMap[propertyName] ;

        // set index to max based on current count.
        int valueIndex = 0;
        for( auto vtimit = propertyValueToIndexMap.cbegin();
                  vtimit != propertyValueToIndexMap.cend();
                  vtimit++ )
        {
            valueIndex += vtimit->second.size();
        }

        for (auto& property : valueSet)
        {
            LOG_DEBUG_F( "Setting (relationship) %s to (pool index) %d\n", property.c_str(), valueIndex /** currentMatrixSize*/ );
            unsigned int poolIndex = valueIndex; // * currentMatrixSize; // skip multiplier if our values are unique across relationship keys (e.g., Relationship0 => 0, Relationship1 => 1, Relationship0 => 2, etc...
            const std::string relationshipId = property;

            // -------------------------------------------------------------------------------------------------------------
            // --- DMB 6-17-2016 The relationship could be in the maps already due to the MAX_DEAD_REL_QUEUE_SIZE
            // --- performance optimization - relationship moved to another node but was not removed from these maps.
            // --- Hence, we need to check if the relationship is already in the map.  Otherwise, adding the relationship
            // --- doesn't increase the size of the map and valueIndex and max_index get off.
            // -------------------------------------------------------------------------------------------------------------
            if( valueToIndexMap.count( relationshipId ) == 0 )
            {
                valueToIndexMap.insert( make_pair( relationshipId, poolIndex ) );
                poolIndexToRelationshipReverseMap[ poolIndex ] = relationshipId;
                LOG_VALID_F( "%s: valueToindexMap.insert( '%s', %d )\n", __FUNCTION__, relationshipId.c_str(), poolIndex );
                LOG_VALID_F( "%s: poolIndexToRelationshipReverMap[ %d ] = '%s'\n", __FUNCTION__, poolIndex, relationshipId.c_str() );
                valueIndex++;
            }
        }
        max_index = valueIndex-1;
    }

    void
    RelationshipGroups::AddProperty(
        const string& property,
        const PropertyValueList_t& values,
        const ScalingMatrix_t& scalingMatrix,
        const string& route
    )
    {
        LOG_VALID_F( "%s( property = '%s', #values = %d, route = '%s' )\n", __FUNCTION__, property.c_str(), values.size(), route.c_str() );
        LOG_DEBUG_F( "Adding Property for property %s (and route %s); values.size() = %d, value[last]=???.\n",
                     property.c_str(),
                     route.c_str(),
                     values.size()
                     //values[values.size()-1].c_str()
                    );
        //CheckForDuplicatePropertyName(property);
        //CheckForValidValueListSize(values);
        //CheckForValidScalingMatrixSize(scalingMatrix, values);

        propertyNameToValuesMap[property] = values;
        // I think we need to rebuild scaling matrix to be N*N 
        AddScalingMatrixToPropertyToMatrixMap(property, scalingMatrix);
    }

    void RelationshipGroups::BuildRouteScalingMatrices( void )
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

    void RelationshipGroups::StoreRouteDecayValues( const RouteToContagionDecayMap_t& contagionDecayRatesByRoute )
    {
        if (contagionDecayRatesByRoute.find(transmissionRouteName) != contagionDecayRatesByRoute.end())
        {
            contagionDecayRate = contagionDecayRatesByRoute.at(transmissionRouteName);
        }
    }

    void
    RelationshipGroups::Build(
        const RouteToContagionDecayMap_t& contagionDecayRatesByRoute,
        int numberOfStrains,
        int numberOfSubstrains
    )
    {
        infectors.clear();
        LOG_DEBUG( "Clearing propertyValueToIndexMap: should be done once each timestep, but NO more.\n" );
        LOG_VALID_F( "%s: propertyValueToindexMap.clear(); poolIndexToRelationshipReverseMap.clear()\n", __FUNCTION__ );
        propertyValueToIndexMap.clear();
        poolIndexToRelationshipReverseMap.clear();

        BuildRouteScalingMatrices(); // The time spent in this grows linearly with number of relationships, but not scary yet. 

        StoreRouteDecayValues(contagionDecayRatesByRoute);

        unsigned int vector_size = 0;
        for (auto& entry : propertyNameToValuesMap)
        {
            vector_size += entry.second.size();
        }
        LOG_VALID_F( "%s: resizing contagion containers to %d\n", __FUNCTION__, vector_size );
        currentContagion.resize( vector_size );
        shedContagion.resize( vector_size );
        infectionRate.resize( vector_size );
    }

    void
    RelationshipGroups::GetGroupMembershipForProperties(
        const RouteList_t& route,
        const tProperties* properties,
        TransmissionGroupMembership_t* membershipOut
    )
    const
    {
        (*membershipOut)[0] = (GroupIndex)0; // map route 0 to index 0 // why?
        membershipOut->clear();

        for (tProperties::const_iterator iProperty = properties->begin(); iProperty != properties->end(); iProperty++)
        {
            const string& propertyName = iProperty->first;
            const string& propertyValue = iProperty->second;

            if (propertyValueToIndexMap.find(propertyName) != propertyValueToIndexMap.end())
            {
                if( propertyValueToIndexMap.at(propertyName).find( propertyValue ) != propertyValueToIndexMap.at(propertyName).end() )
                {
                    // ------------------------------------------------------------------------------------------------------
                    // --- DMB 6-6-2016 In RelationshipManager::RemoveFromPrimaryRelationships(), we gain a very significant
                    // --- reduction in runtime by batching the deleting of the relationships from the transmission group.
                    // --- However, this means that we try to function with entries in the map that do not exist.
                    // --- For example, when a person migrates, their relationship should be removed.  Since it is not removed
                    // --- immediately, the index value could be invalid.  The check makes sure that the index is at least
                    // --- a valid value.  This implies that we need other checks to make sure we don't spread the disease
                    // --- when people are not in a normal relationship (i.e. in the same node).
                    // ------------------------------------------------------------------------------------------------------
                    if( (unsigned int)propertyValueToIndexMap.at(propertyName).at(propertyValue) <= max_index  )
                    {
                        (*membershipOut)[ atoi( propertyValue.c_str() ) ] = propertyValueToIndexMap.at(propertyName).at(propertyValue);
                    }
                    else
                    {
                        // ----------------------------------------------------------------------------------
                        // --- DMB 6-17-2016 - I think the fix in AddPropertyValuesToValueToIndexMap() that 
                        // --- only does the insert if it is not already in the map gets us around this issue.
                        // ----------------------------------------------------------------------------------
                        std::stringstream ss;
                        ss << "The TransmissionGroup map is out of wack.  propertyName=" << propertyName
                           << "  propertyValue=" << propertyValue << "  index=" << propertyValueToIndexMap.at(propertyName).at(propertyValue)
                           << "  max_index=" << max_index;
                        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }
                }
                else
                {
                    // --------------------------------------------------------------------------------------
                    // --- DMB 6-17-2016 One can get here when UpdateGroupMembership() is called.  It can be 
                    // --- called when people are migrating or at other times.  One problem is that a Paused
                    // --- relationship is not in the group but the individual still has the relationship.
                    // --- Hence, this is ok.
                    // --------------------------------------------------------------------------------------
                    LOG_DEBUG_F( "Couldn't find property (name=%s,value=%s) at this node (%d).\n",
                               propertyName.c_str(),
                               propertyValue.c_str(),
                               m_parent->GetRelationshipManager()->GetNode()->GetSuid().data );
                    //throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, (std::string("propertyValueToIndexMap[")+propertyName+"]").c_str(), propertyValue.c_str() );
                }
            }
        }
        /*if (LOG_LEVEL(DEBUG) && properties->size() )
        {
            *msg << std::endl;
            LOG_DEBUG( msg->str().c_str() );
            delete msg;
        }*/
    }

    void RelationshipGroups::UpdatePopulationSize(const TransmissionGroupMembership_t* transmissionGroupMembership, float size_changes, float mc_weight)
    {
        float delta = size_changes * mc_weight;
        populationSize += delta;
        LOG_DEBUG_F( "Increased group population to %f with addition of %f\n", populationSize, delta );
    }

    void
    RelationshipGroups::ExposeToContagion(
        IInfectable* candidate,
        const TransmissionGroupMembership_t* poolMembership,
        float deltaTee
    )
    const
    {
        //LOG_DEBUG_F( "%s: poolMembership size = %d\n", __FUNCTION__, poolMembership->size() );
        // Iterate over all pools. This works for expose, not for deposit.
        for( auto groupIt = poolMembership->begin();
             groupIt != poolMembership->end();
             ++groupIt )
        {
            //GroupIndex poolIndex = poolMembership->begin()->second;
            GroupIndex poolIndex = groupIt->second;
            //IRelationship * pRel = m_parent->GetRelationshipManager()->GetRelationshipById( atoi( poolIndexToRelationshipReverseMap.at( poolIndex ).c_str() ) );

            //float poolInfectionRate = infectionRate[poolIndex];
            act_prob_vec_t act_prob_vec = infectionRate[poolIndex];
            for( auto &entry : act_prob_vec )
            {
                entry.prob_per_act *= candidate->GetInterventionReducedAcquire();
                LOG_DEBUG_F( "prob_per_act = %f after multiplying by %f\n", entry.prob_per_act, candidate->GetInterventionReducedAcquire() );
            }

            //LOG_DEBUG_F( "Pool index %d has contagion value/infectionRate %f\n", poolIndex, poolInfectionRate );

            if (act_prob_vec.size() > 0)
            {
                LOG_INFO_F( "act_prob_vec.size() = %d for index %d\n", act_prob_vec.size(), poolIndex );
                if (candidate != nullptr)
                {
#if 0
                    std::cout << "I am "
                              << dynamic_cast<IIndividualHumanSTI*>(candidate)->GetSuid().data
                              << " and I'm exposing partner in relationship "
                              << poolIndexToRelationshipReverseMap[ poolIndex ]
                              << " with id "
                              << pRel->GetPartner( dynamic_cast<IIndividualHumanSTI*>(candidate) )->GetSuid().data
                              << " to contagion qty "
                              << infectionRate[poolIndex]
                              << std::endl;
#endif
                    IIndividualHumanSTI* sti_human = nullptr;
                    unsigned int receiver = ~0;
                    if (candidate->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_human) == s_OK) {
                        receiver = sti_human->GetSuid().data;
                    }
                    //LogActivity( 0x0EC5B05E, receiver, poolIndex, poolInfectionRate );
                    //candidate->Expose((IContagionPopulation*)&contagionPopulation, deltaTee, TransmissionRoute::TRANSMISSIONROUTE_ALL);
                    release_assert( infectors.size() );
                    if( infectors.find( poolIndex ) == infectors.end() )
                    {
                        throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "infectors", std::to_string(poolIndex).c_str() );
                    }
                    if( dynamic_cast<IIndividualHuman*>(candidate)->IsInfected() == false )
                    {
                        //LOG_DEBUG_F( "Exposing (STI) individual to PROBABILITY set from depositor %d.\n", infectors.at( poolIndex ) );
                        DiscreteContagionPopulation contagionPopulation( act_prob_vec, infectors.at( poolIndex ) );
                        candidate->Expose( &contagionPopulation, deltaTee, TransmissionRoute::TRANSMISSIONROUTE_ALL);
                    }
                }
            }
        }
    }

    float RelationshipGroups::GetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        GroupIndex groupIndex = transmissionGroupMembership->at(0);
        return 0;
    }

    void
    RelationshipGroups::DepositContagion(
        const StrainIdentity* strain,
        float prob,
        const TransmissionGroupMembership_t* poolMembership
    )
    {
#if 0
        if( poolMembership->size() > 1 )
        {
            LOG_WARN_F( "This means code is wrong. Put breakpoint here and debug.\n" );
        }
#endif
        release_assert( poolMembership->size() == 1 );
        auto it = poolMembership->begin();

        GroupIndex poolIndex = it->second;
        LOG_DEBUG_F( "%s: poolIndex = %d\n", __FUNCTION__, poolIndex );
        IRelationship * pRel = m_parent->GetRelationshipManager()->GetRelationshipById( atoi( poolIndexToRelationshipReverseMap[ poolIndex ].c_str() ) );
        if( !pRel )
        {
            std::ostringstream msg;
            msg << "Failed to get relationship pointer for pool index " << poolIndex << std::endl;
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        //release_assert(pRel);
        LOG_DEBUG_F( "Depositing contagion into %d relationship %s (index %d).\n", pRel->GetType(), poolIndexToRelationshipReverseMap[ poolIndex ].c_str(), poolIndex );

        const tRelationshipMembers& members = pRel->GetMembers();
        auto male_partner = *(members.begin());
        auto female_partner = *(members.rbegin());

        IIndividualHumanContext* human1 = nullptr;
        IIndividualHumanContext* human2 = nullptr;

        if (male_partner->QueryInterface(GET_IID(IIndividualHumanContext), (void**)&human1) != s_OK) {
            LOG_ERR("Couldn't get IIndividualHumanContext pointer for male_partner.");
            return;
        }

        if (female_partner->QueryInterface(GET_IID(IIndividualHumanContext), (void**)&human2) != s_OK) {
            LOG_ERR("Couldn't get IIndividualHumanContext pointer for female_partner.");
            return;
        }

        IIndividualHumanEventContext* humanEvent1 = nullptr;
        IIndividualHumanEventContext* humanEvent2 = nullptr;

        if (male_partner->QueryInterface(GET_IID(IIndividualHumanEventContext), (void**)&humanEvent1) != s_OK) {
            LOG_ERR("Couldn't get IIndividualHumanEventContext pointer for male_partner.");
            return;
        }

        if (female_partner->QueryInterface(GET_IID(IIndividualHumanEventContext), (void**)&humanEvent2) != s_OK) {
            LOG_ERR("Couldn't get IIndividualHumanEventContext pointer for female_partner.");
            return;
        }

        LOG_DEBUG_F( "Relationship has individuals %lu (inf=%d) and %lu (inf=%d)\n", human1->GetSuid().data, humanEvent1->IsInfected(), human2->GetSuid().data, humanEvent2->IsInfected() );
        if( humanEvent1->IsInfected() && humanEvent2->IsInfected() )
        {
            LOG_DEBUG_F( "Concordant infected couple; stop wasting time on them.\n" );
            return;
        }

        unsigned int depositor = humanEvent1->IsInfected() ? human1->GetSuid().data : human2->GetSuid().data;

        act_prob_t new_ca_entry;
        new_ca_entry.prob_per_act = prob;
        new_ca_entry.num_acts = 1;

        shedContagion[poolIndex].push_back( new_ca_entry );
        infectors[poolIndex] = depositor; // simplest way to track depositor, always the same for a given pool (without msm)?
        // TBD: this should probably be cleared somewhere
        LOG_DEBUG_F( "Depositing prob %f into shedContagion at index %d for vector size %d and 'depositor' %d.\n",
                     new_ca_entry.prob_per_act, poolIndex, shedContagion[poolIndex].size(), depositor );
/*
        for( auto &act_prob_i : act_prob_vec )
        {
            float approx_amount = act_prob_i.num_acts * act_prob_i.prob_per_act;
            LOG_DEBUG_F( "amount = %f\n", approx_amount);   // Approximate amount for logging

            if (approx_amount >0)
            {
                LogActivity( 0x000ADDED, depositor, poolIndex, approx_amount );

                shedContagion[poolIndex].push_back( act_prob_i );
                
                LOG_DEBUG_F("deposited %f amount of contagion to pool %d, now = %f\n", approx_amount, poolIndex, shedContagion[ poolIndex ] );
            }
            LOG_DEBUG_F( "shedContagion.size = %d\n", shedContagion.size() );
        }
*/
    }

    void 
    RelationshipGroups::CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        // do nothing
    }

    void
    RelationshipGroups::EndUpdate(float infectivityCorrection)
    {
        LOG_DEBUG_F( "%s: shedContagion.size() = %d\n", __FUNCTION__, shedContagion.size() );

        // iterate over all shedContagions, set currentContagion to it.
        for( unsigned int idx=0; idx<shedContagion.size(); idx++ )
        {
            currentContagion[idx] = shedContagion[idx]; 
            infectionRate[idx]    = shedContagion[idx]; 
            //memcpy(infectionRate.data(), currentContagion.data(), shedContagion.size() * sizeof(float));
            //LOG_DEBUG_F( "currentContagion[ %d ] = %f\n", idx, currentContagion[ idx ] );
            LOG_DEBUG_F( "shedContagion at index %d has vector size %d.\n", idx, shedContagion[idx].size() );
            LOG_DEBUG_F( "infectionRate at index %d has vector size %d.\n", idx, infectionRate[idx].size() );
            shedContagion[idx].resize(0);
        }
        //memcpy(currentContagion.data(), shedContagion.data(), shedContagion.size() * sizeof(float));
        //memcpy(infectionRate.data(), currentContagion.data(), currentContagion.size() * sizeof(float));
        //memset(shedContagion.data(), 0, shedContagion.size() * sizeof(float));
    }

    unsigned int RelationshipGroups::_head = 0;
    unsigned int RelationshipGroups::_tail = 0;
    RelationshipGroups::LogEntry RelationshipGroups::_log[GRP_LOG_COUNT];

    // return male id
    NaturalNumber
    DiscreteContagionPopulation::GetInfectorID( void )
    const
    {
        return _infector;
    }

    AntigenId DiscreteContagionPopulation::GetAntigenId( void )
    const
    {
        return _antigen;
    }

    void RelationshipGroups::LogActivity(
        unsigned int _action,
        unsigned int _actor,
        unsigned int _index,
        float _amount
        )
    {
        _log[_head].action = _action;
        _log[_head].actor  = _actor;
        _log[_head].index  = _index;
        _log[_head].amount = _amount;
        _head++;
        _head %= GRP_LOG_COUNT;
        if (_head == _tail) {
            _tail++;
            _tail %= GRP_LOG_COUNT;
        }
    }
#endif
}
