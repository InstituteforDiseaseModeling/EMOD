/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "NodeSet.h"
#if USE_BOOST_SERIALIZATION
#include <boost/serialization/export.hpp>
#endif
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"
#include "InterventionEnums.h"

#if USE_BOOST_SERIALIZATION
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Kernel:INodeSet);
BOOST_CLASS_EXPORT(Kernel::NodeSetAll) 
#endif

static const char* _module = "NodeSetAll";

namespace Kernel
{
    // NodeSetAll
    IMPLEMENT_FACTORY_REGISTERED(NodeSetAll)

    IMPL_QUERY_INTERFACE2(NodeSetAll, INodeSet, IConfigurable)

    json::QuickBuilder
    NodeSetAll::GetSchema()
    //const
    {
        return json::QuickBuilder( jsonSchemaBase );
    }

    bool
    NodeSetAll::Configure(
        const Configuration * pInputJson
    )
    {
        return true; // nothing to configure
    }

    bool
    NodeSetAll::Contains(
        INodeEventContext *ndc
    )
    {
        return true;
    }

#if USE_JSON_SERIALIZATION
    void NodeSetAll::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();
        root->EndObject();
    }

    void NodeSetAll::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
    }
#endif
}
