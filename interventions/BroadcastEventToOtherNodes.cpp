/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "BroadcastEventToOtherNodes.h"

#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "MathFunctions.h"
#include "Migration.h"

static const char * _module = "BroadcastEventToOtherNodes";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(BroadcastEventToOtherNodes)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(BroadcastEventToOtherNodes)

    IMPLEMENT_FACTORY_REGISTERED(BroadcastEventToOtherNodes)

    bool BroadcastEventToOtherNodes::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Event_Trigger", &event_trigger, BETON_Event_Trigger_DESC_TEXT, NO_TRIGGER_STR );
        initConfigTypeMap( "Include_My_Node", &include_my_node, BETON_Include_My_Node_DESC_TEXT, false );

        initConfig( "Node_Selection_Type", node_selection_type, inputJson, MetadataDescriptor::Enum("Node_Selection_Type", BETON_Node_Selection_Type_DESC_TEXT, MDD_ENUM_ARGS(NodeSelectionType)) );
        if( (node_selection_type == NodeSelectionType::DISTANCE_ONLY         ) ||
            (node_selection_type == NodeSelectionType::DISTANCE_AND_MIGRATION) ||
            JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Max_Distance_To_Other_Nodes_Km", &max_distance_km, BETON_Distance_DESC_TEXT, 0, FLT_MAX, FLT_MAX );
        }

        bool ret = JsonConfigurable::Configure( inputJson );

        if( event_trigger == NO_TRIGGER_STR )
        {
            LOG_WARN_F("BroadcastEventToOtherNodes was configured with NoTrigger as the Broadcast_Event.  This special event will not be broadcast.");
        }
        return ret;
    }

    BroadcastEventToOtherNodes::BroadcastEventToOtherNodes()
        : parent( nullptr )
        , p_node_context( nullptr )
        , event_trigger( NO_TRIGGER_STR )
        , include_my_node( false )
        , node_selection_type( NodeSelectionType::DISTANCE_ONLY )
        , max_distance_km( 0.0 )
    {
    }

    BroadcastEventToOtherNodes::BroadcastEventToOtherNodes( const BroadcastEventToOtherNodes& master )
        :BaseIntervention( master )
        , parent( nullptr )
        , p_node_context( nullptr )
        , event_trigger( master.event_trigger )
        , include_my_node( master.include_my_node )
        , node_selection_type( master.node_selection_type )
        , max_distance_km( master.max_distance_km )
    {
    }

    void BroadcastEventToOtherNodes::Update( float dt )
    {
        p_node_context->GetParent()->DistributeEventToOtherNodes( event_trigger, this );

        // expire the intervention
        expired = true;
    }

    void BroadcastEventToOtherNodes::SetContextTo(IIndividualHumanContext *context)
    { 
        parent = context; 
        p_node_context = parent->GetEventContext()->GetNodeEventContext()->GetNodeContext();
    }

    bool BroadcastEventToOtherNodes::Qualifies( const INodeInfo& rni ) const
    {
        // ----------------------------------------------------------
        // --- If the user doesn't indicate that they want this event
        // --- going to their own node, say it doesn't qualify
        // ----------------------------------------------------------
        if( p_node_context->GetSuid() == rni.GetSuid() )
        {
            return include_my_node ;
        }

        bool qualifies = true ;
        if( (node_selection_type == NodeSelectionType::DISTANCE_ONLY         ) ||
            (node_selection_type == NodeSelectionType::DISTANCE_AND_MIGRATION) )
        {
            float distance_to_node_km = CalculateDistanceKm( p_node_context->GetLongitudeDegrees(), p_node_context->GetLatitudeDegrees(),
                                                             rni.GetLongitudeDegrees(), rni.GetLatitudeDegrees() );
            qualifies = (distance_to_node_km <= max_distance_km);
        }

        if( qualifies && ((node_selection_type == NodeSelectionType::MIGRATION_NODES_ONLY  ) ||
                          (node_selection_type == NodeSelectionType::DISTANCE_AND_MIGRATION) ) )
        {
            if( p_node_context->GetMigrationInfo() == nullptr )
            {
                qualifies = false ;
            }
            else
            {
                const std::vector<suids::suid>& r_nodes = p_node_context->GetMigrationInfo()->GetReachableNodes();
                const std::vector<MigrationType::Enum>& r_mig_types = p_node_context->GetMigrationInfo()->GetMigrationTypes();

                release_assert( r_nodes.size() == r_mig_types.size() );

                qualifies = false;
                for( int i = 0 ; !qualifies && (i < r_nodes.size()) ; i++ )
                {
                    if( (r_nodes[i] == rni.GetSuid()) && ((r_mig_types[i] == MigrationType::LOCAL_MIGRATION) || 
                                                          (r_mig_types[i] == MigrationType::REGIONAL_MIGRATION)) )
                    {
                        qualifies = true ;
                    }
                }
            }
        }
        if( qualifies )
        {
            LOG_INFO_F("broadcast %s, %d -> %d\n",event_trigger.c_str(),p_node_context->GetExternalID(),rni.GetExternalID());
        }

        return qualifies ;
    }

    REGISTER_SERIALIZABLE(BroadcastEventToOtherNodes);

    void BroadcastEventToOtherNodes::serialize(IArchive& ar, BroadcastEventToOtherNodes* obj)
    {
        BaseIntervention::serialize( ar, obj );
        BroadcastEventToOtherNodes& cal = *obj;
        ar.labelElement("event_trigger"      ) & cal.event_trigger;
        ar.labelElement("include_my_node"    ) & cal.include_my_node;
        ar.labelElement("node_selection_type") & (uint32_t&)cal.node_selection_type;
        ar.labelElement("max_distance_km"    ) & cal.max_distance_km;

        // -------------------------
        // --- Set in SetContextTo()
        // -------------------------
        // parent
        // p_node_context
    }
}
