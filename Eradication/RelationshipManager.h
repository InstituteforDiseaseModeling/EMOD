
#pragma once

#include <map>
#include <list>
#include "IRelationshipManager.h"

namespace Kernel
{
    class ReportSTI;
    struct ISTISimulationContext;
    struct IRelationship;

    class RelationshipManager : public IRelationshipManager
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

        friend class ReportSTI;
    public:
        RelationshipManager( INodeContext* parent = nullptr );
        virtual void Update( float dt ) override; 
        virtual IRelationship * GetRelationshipById( unsigned int relId ) override;
        virtual const tNodeRelationshipType& GetNodeRelationships() const override;
        virtual INodeContext* GetNode() const override; // is this a good idea? Adding for RelationshipGroups to be node aware
        virtual void AddRelationship( IRelationship*, bool isNewRelationship ) override;
        virtual void RemoveRelationship( IRelationship*, bool isLeavingNode, bool addToDelete ) override;
        virtual void ConsummateRelationship( IRelationship* relationship, const CoitalAct& rCoitalAct ) override;

        virtual suids::suid GetNextCoitalActSuid() override;

        virtual void Emigrate( IRelationship* ) override;
        virtual void Immigrate( IRelationship* ) override;

        virtual void RegisterNewRelationshipObserver(IRelationshipManager::callback_t observer) override;
        virtual void RegisterRelationshipTerminationObserver(IRelationshipManager::callback_t observer) override;
        virtual void RegisterRelationshipConsummationObserver(IRelationshipManager::consummated_callback_t observer) override;

    protected:
        tNodeRelationshipType m_NodeRelationships;
        std::set<IRelationship*> m_NodeRelationshipsToDelete;
        INodeContext* m_pNode;
        ISTISimulationContext* m_pStiSim;
        std::list<IRelationshipManager::callback_t> m_RelationshipObserversNew;
        std::list<IRelationshipManager::callback_t> m_RelationshipObserversTerminated;
        std::list<IRelationshipManager::consummated_callback_t> m_RelationshipObserversConsummated;
        std::map< std::string, std::list<unsigned int> > m_DeadRelationshipsByType;

        void notifyObservers(std::list<IRelationshipManager::callback_t>&, IRelationship*);

        DECLARE_SERIALIZABLE(RelationshipManager);
    };
}
