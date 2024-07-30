
#include "stdafx.h"
#include "NodeSet.h"
#include "ObjectFactoryTemplates.h"


SETUP_LOGGING( "NodeSetFactory" )

namespace Kernel
{
    NodeSetFactory* NodeSetFactory::_instance = nullptr;

    template NodeSetFactory* ObjectFactory<INodeSet, NodeSetFactory>::getInstance();
}
