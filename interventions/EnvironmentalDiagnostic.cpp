/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "EnvironmentalDiagnostic.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "SimulationConfig.h"
#include "SimulationEventContext.h"
#include "ISimulationContext.h"
#include "RANDOM.h"

SETUP_LOGGING("EnvironmentalDiagnostic")

namespace Kernel
{
     BEGIN_QUERY_INTERFACE_BODY(EnvironmentalDiagnostic)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(EnvironmentalDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(EnvironmentalDiagnostic)

    EnvironmentalDiagnostic::EnvironmentalDiagnostic()
        : BaseNodeIntervention()
        , sample_threshold(0)
        , base_specificity(1)
        , base_sensitivity(1)
        , negative_diagnosis_event()
        , positive_diagnosis_event()
        , environment_ip_key_value()
    {
         initSimTypes(2, "ENVIRONMENTAL_SIM", "TYPHOID_SIM");
    }

    bool EnvironmentalDiagnostic::Configure(const Configuration * inputJson)
    {
        initConfigTypeMap( "Sample_Threshold", &sample_threshold, ED_Sample_Threshold_DESC_TEXT, 0, FLT_MAX, 0 );
        initConfigTypeMap( "Base_Specificity", &base_specificity, SD_Base_Specificity_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( "Base_Sensitivity", &base_sensitivity, ED_Base_Sensitivity_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( "Environment_IP_Key_Value", &environment_ip_key_value, ED_Environment_IP_Key_Value_DESC_TEXT );
        initConfigTypeMap( "Negative_Diagnostic_Event", &negative_diagnosis_event, ED_Negative_Diagnostic_Event_DESC_TEXT );
        initConfigTypeMap( "Positive_Diagnostic_Event", &positive_diagnosis_event, ED_Positive_Diagnostic_Event_DESC_TEXT );

        bool ret = BaseNodeIntervention::Configure(inputJson);
        if (ret && !JsonConfigurable::_dryrun)
        {
            if (positive_diagnosis_event.IsUninitialized())
            {
                std::stringstream msg;
                msg << "Invalid Configuration for EnvironmentalDiagnostic. For Positive_Diagnostic_Event an event of type NODE must be configured. The NODE event must be in the Custom_Node_Events list.\n" << std::endl;
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, msg.str().c_str());
            }
        }
        return ret;
    }

    EnvironmentalDiagnostic::EnvironmentalDiagnostic(const EnvironmentalDiagnostic& master)
        : BaseNodeIntervention(master)
        , sample_threshold(master.sample_threshold)
        , negative_diagnosis_event(master.negative_diagnosis_event)
        , positive_diagnosis_event(master.positive_diagnosis_event)
        , environment_ip_key_value(master.environment_ip_key_value)
        , base_specificity(master.base_specificity)
        , base_sensitivity(master.base_sensitivity)
    {
    }

    bool
    EnvironmentalDiagnostic::Distribute(
        INodeEventContext *context,
        IEventCoordinator2* pEC 
    )
    {
        bool ret = BaseNodeIntervention::Distribute( context, pEC );
        if( ret )
        {
            performTest();
        }
        return ret;
    }

    void EnvironmentalDiagnostic::performTest()
    {
        release_assert( parent );
        INodeContext* p_node_context = parent->GetNodeContext();
        float sample = 0.0;
        if( environment_ip_key_value.IsValid() )
        {
            sample = p_node_context->GetContagionByRouteAndProperty( "environmental", environment_ip_key_value );
            LOG_INFO_F( "Intervention_Name=%s  IP=%s  sample = %f  sample_threshold=%f\n",
                         GetName().c_str(), environment_ip_key_value.ToString().c_str(), sample, sample_threshold );
        }
        else
        {
            std::map<std::string, float> route_2_contagion = p_node_context->GetContagionByRoute();
            sample = route_2_contagion.at( "environmental" );
            LOG_INFO_F( "Intervention_Name=%s  sample = %f  sample_threshold=%f\n",
                         GetName().c_str(), sample, sample_threshold );
        }

        bool contagion_detected = ( sample > sample_threshold );
        bool report_positive_test = ( ( contagion_detected && ( parent->GetRng()->SmartDraw( base_sensitivity ) ) ) ||
                                      ( !contagion_detected && (parent->GetRng()->SmartDraw( 1.0 - base_specificity ) ) ) );
        LOG_DEBUG_F( "contagion_detected = %d, base_sensitivity = %f, base_specificity = %f, report_positive_test = %d.\n",
                     contagion_detected, base_sensitivity, base_specificity,report_positive_test
                );

        //broadcast node event
        INodeEventBroadcaster* broadcaster = parent->GetNodeContext()->GetParent()->GetSimulationEventContext()->GetNodeEventBroadcaster();
        if( report_positive_test )
        {
            LOG_DEBUG_F( "EnvironmentalDiagnostic tested positive.\n" );
            broadcaster->TriggerObservers( parent, positive_diagnosis_event );
        }
        else if( !negative_diagnosis_event.IsUninitialized() )
        {
            LOG_DEBUG_F( "EnvironmentalDiagnostic tested negative.\n" );
            broadcaster->TriggerObservers( parent, negative_diagnosis_event );
        }

        // expire the intervention
        SetExpired(true);
    }

    void EnvironmentalDiagnostic::Update(float dt)
    {
        if (!BaseNodeIntervention::UpdateNodesInterventionStatus()) return;

        // I _think_ we can just kill this entire function by doing test at distribute but I don't _know_ that yet.
        //performTest();
    }
}
