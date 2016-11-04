/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "NodeSet.h"
#include "NodeEventContext.h"
#include "Exceptions.h"

static const char* _module = "NodeSetNodeList";

namespace Kernel
{
    using namespace std;
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
            try {
                json::QuickInterpreter nodeListJson( nodelist_qi.As<json::Array>() );

                for( int idx=0; idx<nodelist_qi.As<json::Array>().Size(); idx++ )
                {
                    try {
                        auto nodeId = tNodeId(nodeListJson[idx].As<json::Number>());
                        nodelist.push_back( nodeId );
                    }
                    catch( json::Exception)
                    {
                        throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, std::to_string( idx ).c_str(), nodeListJson, "Expected NUMBER" );
                    }
                }
            }
            catch( json::Exception )
            {
                throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), nodelist_qi, "Expected ARRAY" );
            }
        }
    }

    json::QuickBuilder
    NodeListConfig::GetSchema()
    {
        json::QuickBuilder schema = json::QuickBuilder( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:NodeListConfig" );
        schema[ ts ] = json::Array();
        schema[ ts ][0] = json::Object();
        schema[ ts ][0]["Type"] = json::String( "integer" );
        schema[ ts ][0]["Min"] = json::Number( 0 );
        schema[ ts ][0]["Description"] = json::String( "Id of Node" );
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
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize( Archive &ar, NodeSetNodeList& nodeset, const unsigned int v )
    {
        ar & nodeset.nodelist_config;
    }
}
#endif
