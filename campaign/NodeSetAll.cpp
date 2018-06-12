/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "NodeSet.h"

SETUP_LOGGING( "NodeSetAll" )

namespace Kernel
{
    // NodeSetAll 
    IMPLEMENT_FACTORY_REGISTERED(NodeSetAll)
    IMPLEMENT_FACTORY_REGISTERED(NodeSetNodeList)
    IMPLEMENT_FACTORY_REGISTERED(NodeSetPolygon)

    IMPL_QUERY_INTERFACE2(NodeSetAll, INodeSet, IConfigurable)

    bool NodeSetAll::Configure(const Configuration * pInputJson)
    {
        return true; // nothing to configure
    }

    bool NodeSetAll::Contains(INodeEventContext *ndc)
    {
        return true;
    }

    std::vector<ExternalNodeId_t> NodeSetAll::IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids)
    {
        std::vector<ExternalNodeId_t> dummy;
        return dummy;
    }

    REGISTER_SERIALIZABLE(NodeSetAll);

    void NodeSetAll::serialize(IArchive& ar, NodeSetAll* obj)
    {
        // Nothing to do here. NodeSetAll does its work in Contains()
    }
}
