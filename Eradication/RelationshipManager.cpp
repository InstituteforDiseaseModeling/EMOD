/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "RelationshipManager.h"
#include "IRelationship.h"
#include "Debug.h"

static const char * _module = "RelationshipMgr";

static void
    howlong(
        clock_t before,
        const char * label = "NA"
    )
{
    clock_t after = clock();
    clock_t diff = after - before;
    float thediff = diff/(float)CLOCKS_PER_SEC;

    //std::cout << label << " took " << thediff << " seconds." << std::endl;
}

namespace Kernel
{
    RelationshipManager::RelationshipManager( INodeContext* parent )
        : nodeRelationships()
        , relationshipListsForMP()
        , _node(parent)
        , nodePools(nullptr)
    {
    }

/*
    double RelationshipManager::CND(double d)
    {
        const double       A1 = 0.31938153;
        const double       A2 = -0.356563782;
        const double       A3 = 1.781477937;
        const double       A4 = -1.821255978;
        const double       A5 = 1.330274429;
        const double RSQRT2PI = 0.39894228040143267793994605993438;

        double
        K = 1.0 / (1.0 + 0.2316419 * fabs(d));

        double
        cnd = RSQRT2PI * exp(- 0.5 * d * d) *
              (K * (A1 + K * (A2 + K * (A3 + K * (A4 + K * A5)))));

        if (d > 0)
            cnd = 1.0 - cnd;

        return cnd;
    }
*/

    //static
/*
    float RelationshipManager::selectAgeFromCDF( float targetAge )
    {
        float draw = Environment::getInstance()->RNG->e();
        float cnd = CND( draw-0.5 );
        float age_offset = (cnd*20*DAYSPERYEAR) - 10*DAYSPERYEAR;
        float rel_age = targetAge + age_offset;
        return rel_age;
    }
*/

/*
    RelationshipManager::tAgeBasedMarket::iterator
    RelationshipManager::SelectByAge(
        float targetAge,
        tAgeBasedMarket &candidatesByAge,
        IIndividualHumanSTI** partnerOut
    )
    {
        *partnerOut = NULL;
        float targetPartnerAge = selectAgeFromCDF( targetAge );
        LOG_DEBUG_F( "%s: Searching sorted-by-age individuals with %d people.\n", __FUNCTION__, candidatesByAge.size() );

        for( auto it = candidatesByAge.begin();
                  it != candidatesByAge.end();
                  ++it )
        {
            float age = it->first;
            if( age > targetPartnerAge )
            {
                // LOG_DEBUG_F( "%s: Let's choose this one\n", __FUNCTION__ );
                (*partnerOut) = it->second;
                return it;
            }
        }

        // TBD: failure control path. Throw exception?
        // We're hitting this, throw "SelectByAge() isn't returning anything! =:^O";
        return candidatesByAge.end();
    }
*/

//    void RelationshipManager::Update( Node::individual_human_list_t &individualHumans, ITransmissionGroups* parent, float dt )
    void RelationshipManager::Update( list<IIndividualHuman*>& individualHumans, ITransmissionGroups* parent, float dt )
    {
        clock_t before = clock();
        release_assert( parent );
        nodePools = parent; // don't need to do this over and over do we?

        // update all existing relationships
        LOG_INFO_F( "%s: Updating %d relationships\n", __FUNCTION__, nodeRelationships.size() );
        clock_t before_update = clock();
        for( auto it = nodeRelationships.begin();
                  it != nodeRelationships.end();
                   )
        {
            auto relId = it->first;
            IRelationship* pRel = it->second;
            it++;
            LOG_DEBUG_F( "%s: Updating relationship %d at node %lu\n", __FUNCTION__, pRel->GetId(), _node->GetSuid().data );
            if( pRel->Update( dt ) == false )
            {
                // Need to notify individuals in relationship and also nodePools should be destroyed
                // May be false because someone died.
                pRel->terminate( this );
                delete pRel;
            }
            else
            {
                LOG_DEBUG_F( "%s: relationship %d is ongoing. No action.\n", __FUNCTION__, pRel->GetId() );
            }
        }
        //howlong( before_update, "update_rels" );

        unsigned int new_marriages_counter = 0;
        unsigned int new_hookups_counter = 0;
        unsigned int new_affairs_counter = 0;

        //howlong( before_build, "Build" );
        LOG_INFO_F( "%s: END Pair-Forming System update: %d new marriages, %d new hookups, %d new affairs: %d total relationships.\n", __FUNCTION__, new_marriages_counter, new_hookups_counter, new_affairs_counter, nodeRelationships.size() );
        howlong( before, "RM::Update" );
    }

    void
    RelationshipManager::AddRelationship(
        IRelationship* new_relationship
    )
    {
        LOG_INFO_F("%s( 0x%08X )\n", __FUNCTION__, new_relationship);
        nodeRelationships[ new_relationship->GetId() ] = new_relationship;
        new_relationship->Initialize( this );
        const string& propertyKey  = new_relationship->GetPropertyKey();
        const string& propertyName = new_relationship->GetPropertyName();
        AddToPrimaryRelationships( propertyKey, propertyName ); 
        notifyObservers(new_relationship_observers, new_relationship);
    }

#define MAX_DEAD_REL_QUEUE_SIZE 200 // found by sweeping, might make config param

    void
    RelationshipManager::RemoveRelationship(
        IRelationship* relationship
        )
    {
        notifyObservers(relationship_termination_observers, relationship);

        nodeRelationships.erase( relationship->GetId() );

        // Instead of deleting the relationship string from list each time, we batch them up
        // and do a batch delete. This is a big performance gain. We do the batch delete by
        // transferring all elements from live list into new list one at a time, checking if
        // they are in the (sorted) dead list first. 
        auto thisRelTypeKey = relationship->GetPropertyKey();
        auto &deadRelsThisType = dead_relationships_by_type[ thisRelTypeKey ];

        // performance killing sanity check. Is this already in the morgue?
        //release_assert( std::find( deadRelsThisType.begin(), deadRelsThisType.end(), atoi( relationship->GetPropertyName().c_str() ) ) == deadRelsThisType.end() );

        deadRelsThisType.push_back( atoi( relationship->GetPropertyName().c_str() ) );

        //if( deadRelsThisType.size() > GET_CONFIGURABLE(SimulationConfig)->maximum_dead_relationship_queue_size )
        if( deadRelsThisType.size() > MAX_DEAD_REL_QUEUE_SIZE )
        {
            // Doing batch delete of dead relationships for slot
            deadRelsThisType.sort();
            std::list< std::string > newList;
            auto& current = relationshipListsForMP.at( thisRelTypeKey );
            while( current.size() > 0 )
            {
                auto rel_name = current.front();
                auto rel_name_as_int = atoi( rel_name.c_str() );
            //    auto next_dead_rel_name = deadRelsThisType.front();

                current.pop_front(); // removed from current, either dead or gets put in newList
                if( deadRelsThisType.size() > 0 && rel_name_as_int == deadRelsThisType.front() )
                {
                    deadRelsThisType.pop_front();
                }
                else
                {
                    newList.push_back( rel_name );
                }
            }
            relationshipListsForMP[ thisRelTypeKey ] = newList;
        }
    }

    void
    RelationshipManager::ConsummateRelationship(
        IRelationship* relationship,
        unsigned int acts
    )
    {
        // Notify once per act
        for( unsigned int a = 0; a < acts; a++ )
        {
            notifyObservers(relationship_consummation_observers, relationship);
        }
    }

    void RelationshipManager::RegisterNewRelationshipObserver(IRelationshipManager::callback_t observer)
    {
        new_relationship_observers.push_back(observer);
    }

    void RelationshipManager::RegisterRelationshipTerminationObserver(IRelationshipManager::callback_t observer)
    {
        relationship_termination_observers.push_back(observer);
    }

    void RelationshipManager::RegisterRelationshipConsummationObserver(IRelationshipManager::callback_t observer)
    {
        relationship_consummation_observers.push_back(observer);
    }

    void RelationshipManager::notifyObservers(std::list<IRelationshipManager::callback_t>& observers, IRelationship* new_relationship)
    {
        if (observers.size() > 0)
        {
            for (auto observer : observers) {
                observer(new_relationship);
            }
        }
    }

    IRelationship * 
    RelationshipManager::GetRelationshipById(
        unsigned int relId
    )
    {
        release_assert( relId ); // sometimes gets called with 0, bad.
        // TBD add error handling.
        if( nodeRelationships.find( relId ) == nodeRelationships.end() )
        {
            LOG_WARN_F( "%s: Failed to find relationship %d in the container of %d nodeRelationships at this node. Is this person an immigrant?\n", __FUNCTION__, relId, nodeRelationships.size() );
            return NULL;
        }

        return nodeRelationships.at( relId );
    }

    const tNodeRelationshipType&
    RelationshipManager::GetNodeRelationships()
    const
    {
        return nodeRelationships;
    }

    void
    RelationshipManager::AddToPrimaryRelationships(
        const std::string& propertyKey,
        const std::string& propertyValue
    )
    {
        ScalingMatrix_t scalingMatrix; // { 1 }
        MatrixRow_t matrixRow;
        matrixRow.push_back( 1.0f );
        scalingMatrix.push_back( matrixRow );
        relationshipListsForMP[ propertyKey ].push_back( propertyValue.c_str() );
        nodePools->AddProperty( propertyKey.c_str(), relationshipListsForMP[ propertyKey ], scalingMatrix, "contact" );
    }

    INodeContext*
    RelationshipManager::GetNode()
    const
    {
        return _node;
    }
}
