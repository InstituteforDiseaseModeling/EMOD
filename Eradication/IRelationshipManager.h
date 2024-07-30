
#pragma once
#include "IdmApi.h"
#include <map>
#include <list>
#include <string>

#include "IIndividualHuman.h"

using namespace std;

namespace Kernel
{
    class CoitalAct;
    struct IRelationship;
    typedef map< unsigned int, IRelationship* > tNodeRelationshipType;

    struct IDMAPI IRelationshipManager : ISerializable
    {
        virtual void Update( float dt ) = 0;
        virtual IRelationship* GetRelationshipById( unsigned int id ) = 0;
        virtual const tNodeRelationshipType& GetNodeRelationships() const = 0;
        virtual INodeContext* GetNode() const = 0;
        virtual void AddRelationship( IRelationship*, bool isNewRelationship ) = 0;
        virtual void RemoveRelationship( IRelationship*, bool isLeavingNode, bool addToDelete ) = 0;
        virtual void ConsummateRelationship( IRelationship*, const CoitalAct& rCoitalAct ) = 0;

        virtual suids::suid GetNextCoitalActSuid() = 0;

        virtual void Emigrate( IRelationship* ) = 0;
        virtual void Immigrate( IRelationship* ) = 0;

        typedef std::function<void(IRelationship*)> callback_t;
        virtual void RegisterNewRelationshipObserver(callback_t observer) = 0;
        virtual void RegisterRelationshipTerminationObserver(callback_t observer) = 0;

        typedef std::function<void( IRelationship*, const CoitalAct& )> consummated_callback_t;
        virtual void RegisterRelationshipConsummationObserver( consummated_callback_t observer) = 0;
    };
}
