/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "NodeTBHIV.h"
#include "NodeEventContextHost.h" //for node level trigger
#include "IndividualCoInfection.h"
#include "SimulationConfig.h"
#include "TBHIVParameters.h"
#include "EventTrigger.h"

SETUP_LOGGING( "NodeTBHIV" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(NodeTBHIV, NodeTB)
        HANDLE_INTERFACE( INodeTBHIV )
    END_QUERY_INTERFACE_DERIVED(NodeTBHIV, NodeTB)

    NodeTBHIV::~NodeTBHIV(void)
    {
        //delete HIVCoinfectionDistribution;
        //delete HIVMortalityDistribution;
    }

    NodeTBHIV::NodeTBHIV() : NodeTB() { }

    NodeTBHIV::NodeTBHIV(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
    : NodeTB(_parent_sim, externalNodeId, node_suid)
    , HIVCoinfectionDistribution( nullptr )
    , HIVMortalityDistribution( nullptr )
    {
    }

    NodeTBHIV *NodeTBHIV::CreateNode(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
    {
        NodeTBHIV *newnode = _new_ NodeTBHIV(_parent_sim, externalNodeId, node_suid);
        newnode->Initialize();
        return newnode;
    }

    void NodeTBHIV::Initialize()
    {
        NodeTB::Initialize();
    }

    bool NodeTBHIV::Configure( const Configuration* config )
    {   
        return NodeTB::Configure( config );
    }

    void NodeTBHIV::RegisterObservers()
    {
        cd4observer = _new_ CD4TrajectoryChangeObserver();
        event_context_host->RegisterObserver( cd4observer, EventTrigger::StartedART );
        event_context_host->RegisterObserver( cd4observer, EventTrigger::StoppedART );
    }

    void NodeTBHIV::SetNewInfectionState(InfectionStateChange::_enum inf_state_change, IndividualHuman *ih)
    {
        // Trigger any node level HTI

        //  Latent infection that became active-presymptomatic
        if ( inf_state_change == InfectionStateChange::TBActivationPresymptomatic )   
        {
            event_context_host->TriggerObservers(ih->GetEventContext(), EventTrigger::TBActivationPresymptomatic);
        }
        //  Active presymptomatic infection to active symptomatic
        else if( inf_state_change == InfectionStateChange::TBActivation ||
                 inf_state_change == InfectionStateChange::TBActivationSmearPos ||
                 inf_state_change == InfectionStateChange::TBActivationSmearNeg ||
                 inf_state_change == InfectionStateChange::TBActivationExtrapulm
               )  
        {            
            event_context_host->TriggerObservers( ih->GetEventContext(), EventTrigger::TBActivation );
        } 
        //  Infection got treatment and is now pending relapse - trigger goes off if you are ON OR OFF DRUGS.
        else if ( inf_state_change == InfectionStateChange::ClearedPendingRelapse )   
        {
            event_context_host->TriggerObservers(ih->GetEventContext(), EventTrigger::TBPendingRelapse);
        }
    }

    IIndividualHuman *NodeTBHIV::createHuman(suids::suid suid, float monte_carlo_weight, float initial_age, int gender)
    {
        return IndividualHumanCoInfection::CreateHuman(this, suid, monte_carlo_weight, initial_age, gender);
    }

    IIndividualHuman *NodeTBHIV::addNewIndividual(float monte_carlo_weight, float initial_age, int gender, int initial_infection_count, float immparam, float riskparam, float mighet)
    {
        auto tempind = NodeAirborne::addNewIndividual(monte_carlo_weight, initial_age, gender, initial_infection_count, immparam, riskparam, mighet);

        IIndividualHumanTB * tbhivp = NULL;
        if (s_OK != tempind->QueryInterface(GET_IID(IIndividualHumanTB), (void **) &tbhivp ) )
        {
            throw QueryInterfaceException(__FILE__ ,__LINE__, __FUNCTION__, "tempind", "IndividualHuman", "IndividualHumanTB");
        }

        return tempind;
    }

    bool CD4TrajectoryChangeObserver::notifyOnEvent(IIndividualHumanEventContext *context, const EventTrigger& trigger)
    {
        if (trigger == EventTrigger::StartedART || trigger == EventTrigger::StoppedART)
        {
            IIndividualHumanCoInfection *p_human_co = nullptr;
            if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanCoInfection), (void**)&p_human_co))
            {
                throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanCoInfection", "IIndividualHumanEventContext");
            }
            
            // now adjust the CD4 trajectories and timers accordingly for the latently infected

            if (p_human_co->HasLatentInfection())
            {
                // order of logging matters (for SFTs). Keep this before function call.
                LOG_VALID_F( "%s: Individual %lu has event %s.\n", __FUNCTION__, context->GetSuid().data, trigger.ToString().c_str() );
                p_human_co->LifeCourseLatencyUpdateAll();
            }
        }
        else
        {
            return false;
        }

        return true;
    }

    void NodeTBHIV::LoadOtherDiseaseSpecificDistributions()
    {   
        if (IndividualHumanCoInfectionConfig::enable_coinfection)
        {
            RegisterObservers();
        }

        if( enable_demographics_risk && IndividualHumanCoInfectionConfig::enable_coinfection )
        {
            HIVCoinfectionDistribution = NodeDemographicsDistribution::CreateDistribution( demographics["IndividualAttributes"]["HIVCoinfectionDistribution"], "gender", "time", "age" );
            HIVMortalityDistribution   = NodeDemographicsDistribution::CreateDistribution( demographics["IndividualAttributes"]["HIVTBCoinfMortalityDistribution"], "age", "year" );
        }
    }


    void NodeTBHIV::processEmigratingIndividual(IIndividualHuman* individual)
    {
        NodeAirborne::processEmigratingIndividual(individual);
    }

    IIndividualHuman* NodeTBHIV::processImmigratingIndividual(IIndividualHuman* individual)
    {
        individual = NodeAirborne::processImmigratingIndividual(individual);
        return individual;
    }

    void NodeTBHIV::resetNodeStateCounters( void )
    {        
        for( auto individual : individualHumans )
        {
            dynamic_cast<IndividualHumanCoInfection*>( individual )->ResetCounters();
        }
        NodeTB::resetNodeStateCounters();
    }

    REGISTER_SERIALIZABLE(NodeTBHIV);

    void NodeTBHIV::serialize(IArchive& ar, NodeTBHIV* obj)
    {
        NodeTB::serialize(ar, obj);
    }
}

