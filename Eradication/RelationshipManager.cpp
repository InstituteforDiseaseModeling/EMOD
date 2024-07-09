
#include "stdafx.h"
#include "RelationshipManager.h"
#include "IRelationship.h"
#include "Debug.h"
#include "Log.h"
#include "ISimulationContext.h"
#include "ISTISimulationContext.h"
#include "INodeContext.h"

SETUP_LOGGING( "RelationshipMgr" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(RelationshipManager)
        HANDLE_INTERFACE(IRelationshipManager)
        HANDLE_ISUPPORTS_VIA(IRelationshipManager)
    END_QUERY_INTERFACE_BODY(RelationshipManager)

    RelationshipManager::RelationshipManager( INodeContext* parent )
        : m_NodeRelationships()
        , m_NodeRelationshipsToDelete()
        , m_pNode(parent)
        , m_pStiSim( nullptr )
        , m_RelationshipObserversNew()
        , m_RelationshipObserversTerminated()
        , m_RelationshipObserversConsummated()
    {
        m_pStiSim = dynamic_cast<ISTISimulationContext*>(m_pNode->GetParent());
    }

    void RelationshipManager::Update( float dt )
    {
        clock_t before = clock();

        for( auto p_rel : m_NodeRelationshipsToDelete )
        {
            delete p_rel;
        }
        m_NodeRelationshipsToDelete.clear();

        // update all existing relationships
        LOG_DEBUG_F( "%s: Updating %d relationships\n", __FUNCTION__, m_NodeRelationships.size() );
        //clock_t before_update = clock();
        for( auto it = m_NodeRelationships.begin(); it != m_NodeRelationships.end();  )
        {
            IRelationship* pRel = it->second;
            ++it;
            LOG_DEBUG_F( "%s: Updating relationship %d at node %lu\n", __FUNCTION__, pRel->GetSuid().data, m_pNode->GetSuid().data );

            RelationshipTerminationReason::Enum termination_reason = RelationshipTerminationReason::NA;
            bool partner_terminated = false;
            bool brokeup = false; //i.e. timedout
            if( pRel->GetState() == RelationshipState::PAUSED )
            {
                partner_terminated = m_pStiSim->WasRelationshipTerminatedLastTimestep( pRel->GetSuid() );
            }

            if( !partner_terminated )
            {
                LOG_DEBUG_F( "%s: Updating relationship %d at node %lu\n", __FUNCTION__, pRel->GetSuid().data, m_pNode->GetSuid().data );
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
                RemoveRelationship( pRel, true, true ); // RM owns deleting the object
            }
            else
            {
                LOG_DEBUG_F( "%s: relationship %d is ongoing. No action.\n", __FUNCTION__, pRel->GetSuid().data );
            }
        }
    }

    void RelationshipManager::Emigrate( IRelationship* pRel )
    {
        m_pStiSim->AddEmigratingRelationship( pRel );
    }

    void RelationshipManager::Immigrate( IRelationship* pImmiratingRel )
    {
        auto it = m_NodeRelationships.find( pImmiratingRel->GetSuid().data );

        if( it == m_NodeRelationships.end() )
        {
            m_NodeRelationships[ pImmiratingRel->GetSuid().data ] = pImmiratingRel;
        }
        else
        {
            // if there is an existing object already, then just want to delete this one
            delete pImmiratingRel;
            pImmiratingRel = nullptr;
        }
    }

    void
    RelationshipManager::AddRelationship(
        IRelationship* relationship,
        bool isNewRelationship
    )
    {
        LOG_DEBUG_F("%s( 0x%08X )\n", __FUNCTION__, relationship);
        // if not in the map, add it
        if( m_NodeRelationships.find( relationship->GetSuid().data ) == m_NodeRelationships.end() )
        {
            m_NodeRelationships[ relationship->GetSuid().data ] = relationship;
        }

        if( isNewRelationship )
        {
            notifyObservers(m_RelationshipObserversNew, relationship);
        }
    }

    void RelationshipManager::RemoveRelationship( IRelationship* relationship, bool isLeavingNode, bool addToDelete )
    {
        if( m_NodeRelationships.find( relationship->GetSuid().data ) == m_NodeRelationships.end() )
        {
            return;
        }

        RelationshipState::Enum state = relationship->GetState();
        RelationshipState::Enum previous_state = relationship->GetPreviousState();

        if( state == RelationshipState::TERMINATED )
        {
            notifyObservers(m_RelationshipObserversTerminated, relationship);
        }

        if( isLeavingNode )
        {
            m_NodeRelationships.erase( relationship->GetSuid().data );
            if( addToDelete )
            {
                // When RM doesn't own the deletion, it is because the object has been given
                // to SimulationSTI as part of migration.
                m_NodeRelationshipsToDelete.insert( relationship );
            }
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
            m_pStiSim->AddTerminatedRelationship( m_pNode->GetSuid(), relationship->GetSuid() );
        }
    }

    suids::suid RelationshipManager::GetNextCoitalActSuid()
    {
        return m_pStiSim->GetNextCoitalActSuid();
    }

    void RelationshipManager::ConsummateRelationship( IRelationship* relationship, const CoitalAct& rCoitalAct )
    {
        for( auto observer : m_RelationshipObserversConsummated )
        {
            observer( relationship, rCoitalAct );
        }
    }

    void RelationshipManager::RegisterNewRelationshipObserver(IRelationshipManager::callback_t observer)
    {
        m_RelationshipObserversNew.push_back(observer);
    }

    void RelationshipManager::RegisterRelationshipTerminationObserver(IRelationshipManager::callback_t observer)
    {
        m_RelationshipObserversTerminated.push_back(observer);
    }

    void RelationshipManager::RegisterRelationshipConsummationObserver(IRelationshipManager::consummated_callback_t observer)
    {
        m_RelationshipObserversConsummated.push_back(observer);
    }

    void RelationshipManager::notifyObservers(std::list<IRelationshipManager::callback_t>& observers, IRelationship* new_relationship)
    {
        if (observers.size() > 0)
        {
            for (auto observer : observers)
            {
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
        if( m_NodeRelationships.find( relId ) == m_NodeRelationships.end() )
        {
            LOG_WARN_F( "%s: Failed to find relationship %d in the container of %d m_NodeRelationships at this node. Is this person an immigrant?\n", __FUNCTION__, relId, m_NodeRelationships.size() );
            return nullptr;
        }

        return m_NodeRelationships.at( relId );
    }

    const tNodeRelationshipType&
    RelationshipManager::GetNodeRelationships()
    const
    {
        return m_NodeRelationships;
    }

    INodeContext*
    RelationshipManager::GetNode()
    const
    {
        return m_pNode;
    }

    REGISTER_SERIALIZABLE(RelationshipManager);

    void RelationshipManager::serialize(IArchive& ar, RelationshipManager* obj)
    {
        // ---------------------------------------------------------------------------------------
        // --- NOTE: I'm not really sure that we should be serializing any of these parameters.
        // --- I think that before we serilize, we should remove all of the dead relationships
        // --- Then, when we are deserializing the individuals and their relationships, we should
        // --- should add them to the relaitonship manager.
        // ---------------------------------------------------------------------------------------
        RelationshipManager& mgr = *obj;
        ar.labelElement("m_NodeRelationships");
        size_t count = ar.IsWriter() ? mgr.m_NodeRelationships.size() : -1;
        ar.startArray( count );
        if( ar.IsWriter() )
        {
            for( auto& entry : mgr.m_NodeRelationships )
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
                mgr.m_NodeRelationships[ id ] = nullptr;
            }
        }
        ar.endArray();
    }
}
