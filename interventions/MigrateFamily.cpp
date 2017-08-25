/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MigrateFamily.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "SimulationConfig.h"

SETUP_LOGGING( "MigrateFamily" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MigrateFamily)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MigrateFamily)

    IMPLEMENT_FACTORY_REGISTERED(MigrateFamily)

    bool MigrateFamily::Configure(
        const Configuration * inputJson
    )
    {
        if( !JsonConfigurable::_dryrun && 
            //(EnvPtr->getSimulationConfig() != nullptr) &&
            (GET_CONFIGURABLE(SimulationConfig)->migration_structure == MigrationStructure::NO_MIGRATION) )
        {
            std::stringstream msg;
            msg << _module << " cannot be used when 'Migration_Model' = 'NO_MIGRATION'.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        initConfigTypeMap( "NodeID_To_Migrate_To", &destination_external_node_id,NodeID_To_Migrate_To_DESC_TEXT, 0, INT_MAX, 0 );
        initConfigTypeMap( "Is_Moving", &is_moving, Is_Moving_DESC_TEXT, false );

        duration_before_leaving.Configure( this, inputJson );
        duration_at_node.Configure( this, inputJson );

        bool ret = BaseNodeIntervention::Configure( inputJson );

        if( ret )
        {
            duration_before_leaving.CheckConfiguration();
            duration_at_node.CheckConfiguration();
        }
        return ret;
    }

    MigrateFamily::MigrateFamily()
        : BaseNodeIntervention()
        , destination_external_node_id( 0 )
        , duration_before_leaving()
        , duration_at_node()
        , is_moving( false )
    {
        duration_before_leaving.SetTypeNameDesc( "Duration_Before_Leaving_Distribution_Type", DBL_Type_DESC_TEXT );
        duration_before_leaving.AddSupportedType( DistributionFunction::FIXED_DURATION,       "Duration_Before_Leaving_Fixed",              DBL_Fixed_DESC_TEXT,              "", "" );
        duration_before_leaving.AddSupportedType( DistributionFunction::UNIFORM_DURATION,     "Duration_Before_Leaving_Uniform_Min",        DBL_Uniform_Min_DESC_TEXT,        "Duration_Before_Leaving_Uniform_Max",    DBL_Uniform_Max_DESC_TEXT    );
        duration_before_leaving.AddSupportedType( DistributionFunction::GAUSSIAN_DURATION,    "Duration_Before_Leaving_Gausian_Mean",       DBL_Gausian_Mean_DESC_TEXT,       "Duration_Before_Leaving_Gausian_StdDev", DBL_Gausian_StdDev_DESC_TEXT );
        duration_before_leaving.AddSupportedType( DistributionFunction::EXPONENTIAL_DURATION, "Duration_Before_Leaving_Exponential_Period", DBL_Exponential_Period_DESC_TEXT, "", "" );
        duration_before_leaving.AddSupportedType( DistributionFunction::POISSON_DURATION,     "Duration_Before_Leaving_Poisson_Mean",       DBL_Poisson_Mean_DESC_TEXT,       "", "" );

        duration_at_node.SetTypeNameDesc( "Duration_At_Node_Distribution_Type", DAN_Type_DESC_TEXT );
        duration_at_node.AddSupportedType( DistributionFunction::FIXED_DURATION,       "Duration_At_Node_Fixed",              DAN_Fixed_DESC_TEXT,              "", "" );
        duration_at_node.AddSupportedType( DistributionFunction::UNIFORM_DURATION,     "Duration_At_Node_Uniform_Min",        DAN_Uniform_Min_DESC_TEXT,        "Duration_At_Node_Uniform_Max",    DAN_Uniform_Max_DESC_TEXT    );
        duration_at_node.AddSupportedType( DistributionFunction::GAUSSIAN_DURATION,    "Duration_At_Node_Gausian_Mean",       DAN_Gausian_Mean_DESC_TEXT,       "Duration_At_Node_Gausian_StdDev", DAN_Gausian_StdDev_DESC_TEXT );
        duration_at_node.AddSupportedType( DistributionFunction::EXPONENTIAL_DURATION, "Duration_At_Node_Exponential_Period", DAN_Exponential_Period_DESC_TEXT, "", "" );
        duration_at_node.AddSupportedType( DistributionFunction::POISSON_DURATION,     "Duration_At_Node_Poisson_Mean",       DAN_Poisson_Mean_DESC_TEXT,       "", "" );
    }

    MigrateFamily::MigrateFamily( const MigrateFamily& master )
        : BaseNodeIntervention( master )
        , destination_external_node_id( master.destination_external_node_id )
        , duration_before_leaving( master.duration_before_leaving )
        , duration_at_node( master.duration_at_node )
        , is_moving( master.is_moving )
    {
    }

    bool MigrateFamily::Distribute( INodeEventContext *pNodeEventContext, IEventCoordinator2 *pEC )
    {
        ISimulationContext* p_sim = pNodeEventContext->GetNodeContext()->GetParent();
        if( !p_sim->CanSupportFamilyTrips() )
        {
            std::stringstream msg;
            msg << "Invalid Configuration for Family Trips.  MigrateFamily cannot be used." << std::endl;
            msg << "Migration_Pattern must be SINGLE_ROUND_TRIPS and the 'XXX_Migration_Roundtrip_Probability' must equal 1.0 if that Migration Type is enabled." << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        return BaseNodeIntervention::Distribute( pNodeEventContext, pEC );
    }

    void MigrateFamily::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        INodeContext* p_node_context = parent->GetNodeContext();

        if( !Expired() )
        {
            // expire the intervention
            SetExpired( true );

            suids::suid destination_id = p_node_context->GetParent()->GetNodeSuid( destination_external_node_id );

            float duration_before = duration_before_leaving.CalculateDuration();
            float duration_at     = duration_at_node.CalculateDuration();

            p_node_context->SetWaitingForFamilyTrip( destination_id, MigrationType::INTERVENTION_MIGRATION, duration_before, duration_at, is_moving );
        }
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! TODO - can't do until BaseNodeIntervention is serializable
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //REGISTER_SERIALIZABLE(MigrateFamily);

    //void MigrateFamily::serialize(IArchive& ar, MigrateFamily* obj)
    //{
    //    BaseNodeIntervention::serialize( ar, obj );
    //    MigrateFamily& mf = *obj;
    //    ar.labelElement("destination_external_node_id") & mf.destination_external_node_id;
    //    ar.labelElement("duration_before_leaving"     ) & mf.duration_before_leaving;
    //    ar.labelElement("duration_at_node"            ) & mf.duration_at_node;
    //    ar.labelElement("is_moving"                   ) & mf.is_moving;
    //}
}
