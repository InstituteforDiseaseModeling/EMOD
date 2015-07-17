/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <map>
#include <list>
#include "IRelationshipManager.h"

namespace Kernel
{
    class ReportSTI;
    class RelationshipManager : public IRelationshipManager
    {
        friend class ReportSTI;
    public:
        RelationshipManager( INodeContext* parent );
        void Update( list<IIndividualHuman*>& individualHumans, ITransmissionGroups* groups, float dt ); 
        IRelationship * GetRelationshipById( unsigned int relId );
        const tNodeRelationshipType& GetNodeRelationships() const;
        void AddToPrimaryRelationships( const std::string& propertyKey, const std::string& propertyValue );
        INodeContext* GetNode() const; // is this a good idea? Adding for RelationshipGroups to be node aware
        void AddRelationship(IRelationship*);
        void RemoveRelationship( IRelationship* );
        void ConsummateRelationship( IRelationship* relationship, unsigned int acts );

        virtual void RegisterNewRelationshipObserver(IRelationshipManager::callback_t observer);
        virtual void RegisterRelationshipTerminationObserver(IRelationshipManager::callback_t observer);
        virtual void RegisterRelationshipConsummationObserver(IRelationshipManager::callback_t observer);

    protected:
        tNodeRelationshipType nodeRelationships;
        std::map< const std::string, PropertyValueList_t > relationshipListsForMP;
        INodeContext* _node;
        ITransmissionGroups * nodePools;
        std::list<IRelationshipManager::callback_t> new_relationship_observers;
        std::list<IRelationshipManager::callback_t> relationship_termination_observers;
        std::list<IRelationshipManager::callback_t> relationship_consummation_observers;
        std::map< std::string, std::list<unsigned int> > dead_relationships_by_type;

        void notifyObservers(std::list<IRelationshipManager::callback_t>&, IRelationship*);
    };
}
