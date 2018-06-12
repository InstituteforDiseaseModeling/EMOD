/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include <list>
#include "IRelationshipManager.h"

namespace Kernel
{
    class RelationshipGroups;
    class ReportSTI;
    class RelationshipManager : public IRelationshipManager
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

        friend class ReportSTI;
    public:
        RelationshipManager( INodeContext* parent = nullptr );
        virtual void Update( list<IIndividualHuman*>& individualHumans, ITransmissionGroups* groups, float dt ) override; 
        virtual IRelationship * GetRelationshipById( unsigned int relId ) override;
        virtual const tNodeRelationshipType& GetNodeRelationships() const override;
        virtual INodeContext* GetNode() const override; // is this a good idea? Adding for RelationshipGroups to be node aware
        virtual void AddRelationship( IRelationship*, bool isNewRelationship ) override;
        virtual void RemoveRelationship( IRelationship*, bool leavingNode ) override;
        virtual void ConsummateRelationship( IRelationship* relationship, unsigned int acts ) override;

        virtual IRelationship* Emigrate( IRelationship* ) override;
        virtual IRelationship* Immigrate( IRelationship* ) override;

        virtual void RegisterNewRelationshipObserver(IRelationshipManager::callback_t observer) override;
        virtual void RegisterRelationshipTerminationObserver(IRelationshipManager::callback_t observer) override;
        virtual void RegisterRelationshipConsummationObserver(IRelationshipManager::callback_t observer) override;

    protected:
        virtual void AddToPrimaryRelationships( IRelationship* relationship );
        virtual void RemoveFromPrimaryRelationships( IRelationship* relationship );

        tNodeRelationshipType nodeRelationships;
        std::map< std::string, list<uint32_t> > relationshipListsForMP;
        INodeContext* _node;
        RelationshipGroups * nodePools;
        std::list<IRelationshipManager::callback_t> new_relationship_observers;
        std::list<IRelationshipManager::callback_t> relationship_termination_observers;
        std::list<IRelationshipManager::callback_t> relationship_consummation_observers;
        std::map< std::string, std::list<unsigned int> > dead_relationships_by_type;

        void notifyObservers(std::list<IRelationshipManager::callback_t>&, IRelationship*);

        DECLARE_SERIALIZABLE(RelationshipManager);
    };
}
