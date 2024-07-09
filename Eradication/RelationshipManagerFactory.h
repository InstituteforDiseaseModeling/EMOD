
#pragma once

#include "IdmApi.h"
#include "IRelationshipManager.h"

namespace Kernel {

    class RelationshipManagerFactory {
    public:
        IDMAPI static IRelationshipManager* CreateManager(INodeContext *parent);
    };
}