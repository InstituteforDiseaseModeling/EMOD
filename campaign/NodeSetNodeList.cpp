/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "NodeSet.h"
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"

static const char* _module = "NodeSetNodeList";

namespace Kernel
{
    using namespace std;
    // NodeSet
    //INodeSetFactory * NodeSetFactory::_instance = NULL;

    // NodeSetNodeList
    IMPLEMENT_FACTORY_REGISTERED(NodeSetNodeList)

    IMPL_QUERY_INTERFACE2(NodeSetNodeList, INodeSet, IConfigurable)

    void
    NodeListConfig::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        // vertices should be valid json array
        if( nodelist.size() == 0 )
        {
            // haven't parsed raw list yet.
            // Go through list and parse out.
            json::QuickInterpreter nodelist_qi( (*inputJson)[key] );
            json::QuickInterpreter nodeListJson( nodelist_qi.As<json::Array>() );
            for( int idx=0; idx<nodelist_qi.As<json::Array>().Size(); idx++ )
            {
                auto nodeId = (tNodeId) nodeListJson[idx].As<json::Number>();
                nodelist.push_back( nodeId );
            }
        }
    }

    json::QuickBuilder
    NodeListConfig::GetSchema()
    {
        json::QuickBuilder schema = json::QuickBuilder( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:NodeListConfig" );
        schema[ ts ] = json::Array();
        schema[ ts ][0] = json::Object();
        schema[ ts ][0]["Type"] = json::String( "Integer" );
        schema[ ts ][0]["Min"] = json::Number( 0 );
        schema[ ts ][0]["Description"] = json::String( "Id of Node" );
        return schema;
    }


    json::QuickBuilder
    NodeSetNodeList::GetSchema()
    {
        json::QuickBuilder schema = json::QuickBuilder( jsonSchemaBase );
        return schema;
    }
   
    bool
    NodeSetNodeList::Configure(
        const Configuration * inputJson
    )
    {
        initConfigComplexType( "Node_List", &nodelist_config, Node_List_DESC_TEXT );
        return JsonConfigurable::Configure( inputJson );
    }

    //
    // NodeSetNodeList class methods
    //
    bool
    NodeSetNodeList::Contains(
        INodeEventContext *nec
    )
    {

        LOG_DEBUG_F("node id = %d\n", nec->GetId().data);

        // We have external ids, but Node never exposes this id because it will get abused, so go through encapsulating method.
        return nec->IsInExternalIdSet( nodelist_config.nodelist );
    }

#if USE_JSON_SERIALIZATION

    // IJsonSerializable Interfaces
    void NodeSetNodeList::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();
        root->Insert("nodelist");
        //helper->JSerialize(nodelist_config, root);
        root->EndObject();
    }

    void NodeSetNodeList::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
    }
#endif
}

#if USE_BOOST_SERIALIZATION
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Kernel:INodeSet);
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT(Kernel::NodeSetNodeList)

namespace Kernel {
    template<class Archive>
    void serialize(
        Archive &ar,
        NodeSetNodeList& nodeset,
        const unsigned int v
    )
    {
        boost::serialization::void_cast_register<NodeSetNodeList, INodeSet>();
        ar & nodeset.nodelist_config;
    }
}
#endif
