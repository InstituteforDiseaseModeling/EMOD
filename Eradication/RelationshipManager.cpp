/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "RelationshipManager.h"
#include "IRelationship.h"
#include "Debug.h"
#include "Log.h"
#include "ISTISimulationContext.h"

static const char * _module = "RelationshipMgr";

static void
    howlong(
        clock_t before,
        const char * label = "NA"
    )
{
    //clock_t after = clock();
    //clock_t diff = after - before;
    //float thediff = diff/(float)CLOCKS_PER_SEC;
    //std::cout << label << " took " << thediff << " seconds." << std::endl;
}

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(RelationshipManager)
        HANDLE_INTERFACE(IRelationshipManager)
        HANDLE_ISUPPORTS_VIA(IRelationshipManager)
    END_QUERY_INTERFACE_BODY(RelationshipManager)

    RelationshipManager::RelationshipManager( INodeContext* parent )
        : nodeRelationships()
        , relationshipListsForMP()
        , _node(parent)
        , nodePools(nullptr)
        , new_relationship_observers()
        , relationship_termination_observers()
        , relationship_consummation_observers()
        , dead_relationships_by_type()
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
        *partnerOut = nullptr;
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

    void RelationshipManager::Update( list<IIndividualHuman*>& individualHumans, ITransmissionGroups* parent, float dt )
    {
        clock_t before = clock();
        release_assert( parent );
        nodePools = parent; // don't need to do this over and over do we?

        // update all existing relationships
        LOG_INFO_F( "%s: Updating %d relationships\n", __FUNCTION__, nodeRelationships.size() );
        //clock_t before_update = clock();
        for( auto it = nodeRelationships.begin();
                  it != nodeRelationships.end();
                   )
        {
            IRelationship* pRel = it->second;
            ++it;
            LOG_DEBUG_F( "%s: Updating relationship %d at node %lu\n", __FUNCTION__, pRel->GetSuid().data, _node->GetSuid().data );

            RelationshipTerminationReason::Enum termination_reason = RelationshipTerminationReason::NOT_TERMINATING;
            bool partner_terminated = false;
            bool brokeup = false; //i.e. timedout
            if( pRel->GetState() == RelationshipState::PAUSED )
            {
                ISTISimulationContext* p_sti_sim = dynamic_cast<ISTISimulationContext*>(_node->GetParent());
                partner_terminated = p_sti_sim->WasRelationshipTerminatedLastTimestep( pRel->GetSuid() );
            }

            if( !partner_terminated )
            {
                LOG_DEBUG_F( "%s: Updating relationship %d at node %lu\n", __FUNCTION__, pRel->GetSuid().data, _node->GetSuid().data );
                brokeup = (pRel->Update( dt ) == false); //i.e. timedout
            }

            if( partner_terminated || brokeup )
            {
                if( partner_terminated )
                {
                    termination_reason = RelationshipTerminationReason::PARTNER_TERMINATED;
                }
                else
                {
                    termination_reason = RelationshipTerminationReason::BROKEUP;
                }
                pRel->Terminate( termination_reason );

                // ----------------------------------------------------------
                // --- IRelationship::Terminate() is responsible for telling
                // --- IRelationshipManager to remove the relationship
                // ----------------------------------------------------------
                delete pRel;
            }
            else
            {
                LOG_DEBUG_F( "%s: relationship %d is ongoing. No action.\n", __FUNCTION__, pRel->GetSuid().data );
            }
        }
        howlong( before, "RM::Update" );
    }

    IRelationship* RelationshipManager::Emigrate( IRelationship* pRel )
    {
        return pRel;
    }

    IRelationship* RelationshipManager::Immigrate( IRelationship* pRel )
    {
        IRelationship* p_return_rel = pRel;
        if( nodeRelationships.find( pRel->GetSuid().data ) != nodeRelationships.end() )
        {
            IRelationship*p_existing_rel = nodeRelationships.at( pRel->GetSuid().data );
            release_assert( p_existing_rel );
            release_assert( pRel->GetSuid().data == p_existing_rel->GetSuid().data );
            p_return_rel = p_existing_rel;
        }
        return p_return_rel;
    }

    void
    RelationshipManager::AddRelationship(
        IRelationship* relationship,
        bool isNewRelationship
    )
    {
        LOG_INFO_F("%s( 0x%08X )\n", __FUNCTION__, relationship);
        // if not in the map, add it
        if( nodeRelationships.find( relationship->GetSuid().data ) == nodeRelationships.end() )
        {
            nodeRelationships[ relationship->GetSuid().data ] = relationship;
        }

        // If we have a normal relationship, we want couple open for disease transmission
        if( relationship->GetState() == RelationshipState::NORMAL )
        {
            AddToPrimaryRelationships( relationship ); 
        }

        if( isNewRelationship )
        {
            notifyObservers(new_relationship_observers, relationship);
        }
    }

    void
    RelationshipManager::RemoveRelationship(
        IRelationship* relationship,
        bool leavingNode
        )
    {
        if( nodeRelationships.find( relationship->GetSuid().data ) == nodeRelationships.end() )
        {
            return;
        }

        RelationshipState::Enum state = relationship->GetState();
        RelationshipState::Enum previous_state = relationship->GetPreviousState();

        if( state == RelationshipState::TERMINATED )
        {
            notifyObservers(relationship_termination_observers, relationship);
        }

        if( leavingNode )
        {
            nodeRelationships.erase( relationship->GetSuid().data );
        }


        // -----------------------------------------------------------------------
        // --- If the previous_state is Normal, then the relationship should be in the pool.
        // --- If the previous_state is Paused, Migrating, or Terminated, then the relationship should NOT be in the pool.
        // -----------------------------------------------------------------------
        if( previous_state == RelationshipState::NORMAL )
        {
            RemoveFromPrimaryRelationships( relationship );
        }

        // ------------------------------------------------------------------------
        // --- Needed so that the other partner can find out that the relationship
        // --- has been terminated.  It could have been by choice or by death.
        // --- If we are terminating the relationship because the partner terminated it,
        // --- then we don't need to tell him.
        // ------------------------------------------------------------------------
        if( (state == RelationshipState::TERMINATED) && 
            (previous_state == RelationshipState::PAUSED) &&
            (relationship->GetTerminationReason() != RelationshipTerminationReason::PARTNER_TERMINATED)
          )
        {
            ISTISimulationContext* p_sti_sim = dynamic_cast<ISTISimulationContext*>(_node->GetParent());
            p_sti_sim->AddTerminatedRelationship( _node->GetSuid(), relationship->GetSuid() );
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
            return nullptr;
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
        IRelationship* relationship
    )
    {
        std::string propertyKey = relationship->GetPropertyKey();
        std::string propertyValue = relationship->GetPropertyName();
        ScalingMatrix_t scalingMatrix; // { 1 }
        MatrixRow_t matrixRow;
        matrixRow.push_back( 1.0f );
        scalingMatrix.push_back( matrixRow );
        relationshipListsForMP[ propertyKey ].push_back( propertyValue.c_str() );
        nodePools->AddProperty( propertyKey.c_str(), relationshipListsForMP[ propertyKey ], scalingMatrix, "contact" );
    }

#define MAX_DEAD_REL_QUEUE_SIZE 0 //200 // found by sweeping, might make config param

    void RelationshipManager::RemoveFromPrimaryRelationships( IRelationship* relationship )
    {
        // Instead of deleting the relationship string from list each time, we batch them up
        // and do a batch delete. This is a big performance gain. We do the batch delete by
        // transferring all elements from live list into new list one at a time, checking if
        // they are in the (sorted) dead list first. 
        auto thisRelTypeKey = relationship->GetPropertyKey();
        auto &deadRelsThisType = dead_relationships_by_type[ thisRelTypeKey ];

        // stop trying to remove relationship if the type isn't there
        if( relationshipListsForMP.count( thisRelTypeKey ) == 0 )
        {
            return;
        }

        // performance killing sanity check. Is this already in the morgue?
        //release_assert( std::find( deadRelsThisType.begin(), deadRelsThisType.end(), atoi( relationship->GetPropertyName().c_str() ) ) == deadRelsThisType.end() );

        deadRelsThisType.push_back( atoi( relationship->GetPropertyName().c_str() ) );

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

            // --------------------------------------------------------------------------------------------
            // ---  Update the transmission group so that this relationship is no longer part of the group
            // --------------------------------------------------------------------------------------------
            ScalingMatrix_t scalingMatrix; // { 1 }
            MatrixRow_t matrixRow;
            matrixRow.push_back( 1.0f );
            scalingMatrix.push_back( matrixRow );
            nodePools->AddProperty( thisRelTypeKey.c_str(), relationshipListsForMP[ thisRelTypeKey ], scalingMatrix, "contact" );
        }
    }

    INodeContext*
    RelationshipManager::GetNode()
    const
    {
        return _node;
    }

    REGISTER_SERIALIZABLE(RelationshipManager);

    void RelationshipManager::serialize(IArchive& ar, RelationshipManager* obj)
    {
        // ---------------------------------------------------------------------------------------
        // --- NOTE: I'm not really sure that we should be serializing any of these parameters.
        // --- I think that before we serilize, we should remove all of the dead relationships
        // --- (this would make dead_relationships_by_type empty).  Then, when we are deserializing 
        // --- the individuals and their relationships, we should should add them to the 
        // --- relaitonship manager.
        // ---------------------------------------------------------------------------------------
        RelationshipManager& mgr = *obj;
        ar.labelElement("nodeRelationships");
        size_t count = ar.IsWriter() ? mgr.nodeRelationships.size() : -1;
        ar.startArray( count );
        if( ar.IsWriter() )
        {
            for( auto& entry : mgr.nodeRelationships )
            {
                unsigned int id = entry.first;
                ar & id;
            }
        }
        else
        {
            for( int i = 0 ; i < count ; i++ )
            {
                unsigned int id = 0;
                ar & id;
                mgr.nodeRelationships[ id ] = nullptr;
            }
        }
        ar.endArray();
 
        ar.labelElement("relationshipListsForMP"    ) & mgr.relationshipListsForMP;
        //_node
        // nodePools
        ar.labelElement("dead_relationships_by_type") & mgr.dead_relationships_by_type;
        //std::list<IRelationshipManager::callback_t> new_relationship_observers;
        //std::list<IRelationshipManager::callback_t> relationship_termination_observers;
        //std::list<IRelationshipManager::callback_t> relationship_consummation_observers;

    }
}
