/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "CoverageByNodeEventCoordinator.h"

#include "NodeEventContext.h"
#include "IIndividualHumanContext.h"
#include "RANDOM.h"

SETUP_LOGGING( "CoverageByNodeEventCoordinator" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(CoverageByNodeEventCoordinator)

    IMPL_QUERY_INTERFACE2(CoverageByNodeEventCoordinator, IEventCoordinator, IConfigurable)

    void
    CoverageByNodeJson::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        try {
            json::QuickInterpreter json_array( (*inputJson)[key].As<json::Array>() );
            for( unsigned int idx = 0; idx < (*inputJson)[key].As<json::Array>().Size(); idx++ )
            {
                json::QuickInterpreter node_coverage_pair( json_array[idx] );
                if (node_coverage_pair.As<json::Array>().Size() != 2)
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Coverage_By_Node parameter needs to be an array of [nodeID,coverage] pairs." );
                }
                uint32_t nodeid = 0;
                float coverage = 0.0f;
                try {
                    nodeid = (uint32_t) node_coverage_pair[0].As<json::Number>();
                }
                catch( const json::Exception & )
                {
                    throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, "0", node_coverage_pair, "Expected NUMBER" );
                }
                try {
                    coverage = (float) node_coverage_pair[1].As<json::Number>();
                }
                catch( const json::Exception & )
                {
                    throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, "1", node_coverage_pair, "Expected NUMBER" );
                }

                LOG_DEBUG_F("Parsing Coverage_By_Node property: nodeid=%d, coverage=%0.2f.\n", nodeid, coverage);

                auto ret = node_coverage_map.insert(std::pair<uint32_t,float>(nodeid,coverage));
                if (ret.second == false)
                {
                    LOG_WARN_F("Duplicate coverage specified for node with ID=%d. Using first coverage value specified.\n", nodeid);
                }
            }
        }
        catch( const json::Exception & )
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), (*inputJson), "Expected ARRAY" );
        }
    }

    json::QuickBuilder
    CoverageByNodeJson::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:CoverageByNode" );
        schema[ ts ] = json::Array();
        schema[ ts ][0] = json::Array();
        schema[ ts ][0][0] = json::Object();
        schema[ ts ][0][0][ "description" ] = json::String( "Node Id" );
        schema[ ts ][0][0][ "type" ] = json::String( "integer" );
        schema[ ts ][0][0][ "min" ] = json::Number( 0 );
        schema[ ts ][0][0][ "max" ] = json::Number( 999999 );
        schema[ ts ][0][1] = json::Object();
        schema[ ts ][0][1][ "description" ] = json::String( "Coverage" );
        schema[ ts ][0][1][ "type" ] = json::String( "float" );
        schema[ ts ][0][1][ "min" ] = json::Number( 0 );
        schema[ ts ][0][1][ "max" ] = json::Number( 1 );
        return schema;
    }

    CoverageByNodeEventCoordinator::CoverageByNodeEventCoordinator()
        : StandardInterventionDistributionEventCoordinator( false )//false=don't use standard demographic coverage
    {
    }

    bool CoverageByNodeEventCoordinator::Configure( const Configuration * inputJson )
    {
        initConfigComplexType("Coverage_By_Node", &coverage_by_node, Coverage_By_Node_DESC_TEXT );
        bool configured = StandardInterventionDistributionEventCoordinator::Configure(inputJson);

        try
        {
            if( !JsonConfigurable::_dryrun )
            {
            }
        }
        catch( const json::Exception &e )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() ); // ( "Coverage_By_Node json problem: Coverage_By_Node is valid json but needs to be an array of (nodeID,coverage) pairs." );
        }

        return configured;
    }

    bool CoverageByNodeEventCoordinator::TargetedIndividualIsCovered(IIndividualHumanEventContext *ihec)
    {
        bool covered=false;

        ExternalNodeId_t nodeid = ihec->GetNodeEventContext()->GetExternalId();

        auto coverage_it = coverage_by_node.node_coverage_map.find(nodeid);
        
        if (coverage_it != coverage_by_node.node_coverage_map.end())
        {
            float coverage = coverage_it->second;
            covered = ihec->GetInterventionsContext()->GetParent()->GetRng()->SmartDraw( coverage );
        }
        else
        {
            LOG_WARN_F("No coverage specified for Node with ID = %d.  Defaulting to zero coverage.\n", nodeid);
        }

        return covered;
    }
}
