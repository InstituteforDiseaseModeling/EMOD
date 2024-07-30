
#pragma once

#include "IAssortivity.h"
#include "IRelationship.h"

namespace Kernel 
{
    class RANDOMBASE;

    class IDMAPI AssortivityFactory
    {
    public:
        static IAssortivity* CreateAssortivity( RelationshipType::Enum relType, RANDOMBASE* prng );
    };
}