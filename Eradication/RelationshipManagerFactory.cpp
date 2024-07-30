
#include "stdafx.h"
#include "RelationshipManagerFactory.h"
#include "RelationshipManager.h"

namespace Kernel
{
    IRelationshipManager* RelationshipManagerFactory::CreateManager(INodeContext *parent)
    {
        IRelationshipManager* manager = _new_ RelationshipManager( parent );
        return manager;
    }
}
