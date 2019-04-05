/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "NodeTB.h"
#include "TransmissionGroupsFactory.h" //for SetupIntranodeTransmission
#include "NodeEventContextHost.h" //for node level trigger
#include "IndividualCoInfection.h"
#include "SimulationConfig.h"
#include "EventTrigger.h"

SETUP_LOGGING( "NodeTB" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(NodeTB, NodeAirborne)
        HANDLE_INTERFACE(INodeTB)
    END_QUERY_INTERFACE_DERIVED(NodeTB, NodeAirborne)


    NodeTB::~NodeTB(void) { }

    NodeTB::NodeTB() : NodeAirborne() { }

    NodeTB::NodeTB(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
        : NodeAirborne(_parent_sim, externalNodeId, node_suid)
    {
    }

    NodeTB *NodeTB::CreateNode(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
    {
        NodeTB *newnode = _new_ NodeTB(_parent_sim, externalNodeId, node_suid);
        newnode->Initialize();

        return newnode;
    }

    bool NodeTB::Configure( const Configuration* config )
    {
        return Node::Configure( config );
    }

    void NodeTB::Initialize()
    {
        NodeAirborne::Initialize();
    }

    IIndividualHuman* NodeTB::createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender)
    {
        return IndividualHumanCoInfection::CreateHuman(this, suid, monte_carlo_weight, initial_age, gender);
    }

    void NodeTB::OnNewInfectionState(InfectionStateChange::_enum inf_state_change, IndividualHuman *ih)
    {
        // Trigger any node level HTI

        IIndividualHumanTB2 *tb_ind= nullptr;
        if( ih->QueryInterface( GET_IID( IIndividualHumanTB2 ), (void**) &tb_ind ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "ih", "IndividualHuman", "IIndividualHumanTB2" );
        }

        switch (inf_state_change)
        {
        //  Latent infection that became active pre-symptomatic
        case InfectionStateChange::TBActivationPresymptomatic:
            event_context_host->TriggerObservers(ih->GetEventContext(), EventTrigger::TBActivationPresymptomatic);
            break;

        //  Active pre-symptomatic infection to active symptomatic
        //NOTE: The infection state change is disaggregated by smear status during the symptomatic phase, but the trigger TBActivation is hooked up to all disease presentations.
        //NOTE: Also if it is a relapse, it does not disaggregate by smear status
        case InfectionStateChange::TBActivation:
        case InfectionStateChange::TBActivationSmearPos:
        case InfectionStateChange::TBActivationSmearNeg:
        case InfectionStateChange::TBActivationExtrapulm:
            if ( tb_ind->HasEverRelapsedAfterTreatment() )
            {
                 event_context_host->TriggerObservers(ih->GetEventContext(), EventTrigger::TBActivationPostRelapse);
            }
            else
            {
                event_context_host->TriggerObservers(ih->GetEventContext(), EventTrigger::TBActivation);
            }
            break;

        //  Infection got treatment and is now pending relapse - trigger goes off if you are ON OR OFF DRUGS.
        case InfectionStateChange::ClearedPendingRelapse:
            event_context_host->TriggerObservers(ih->GetEventContext(), EventTrigger::TBPendingRelapse);
            break;

        // no other infection state change is connected to a trigger, no trigger goes off in this time step
        default:
            //do nothing
            break;
        }
    }

    ITransmissionGroups* NodeTB::CreateTransmissionGroups()
    {
        return TransmissionGroupsFactory::CreateNodeGroups( TransmissionGroupType::StrainAwareGroups, GetRng() );
    }

    void NodeTB::BuildTransmissionRoutes( float contagionDecayRate )
    {
        int max_antigens = InfectionConfig::number_basestrains;
        int max_genomes = InfectionConfig::number_substrains;
        LOG_DEBUG_F("max_antigens %f, max_genomes %f", max_antigens, max_genomes);
        transmissionGroups->Build( contagionDecayRate, max_antigens, max_genomes ); 
    }

    void NodeTB::resetNodeStateCounters(void)
    {
        NodeAirborne::resetNodeStateCounters();
        incident_counter = 0.0f;
        MDR_incident_counter = 0.0f;
        MDR_evolved_incident_counter = 0.0f;
        MDR_fast_incident_counter = 0.0f;
    }

    IIndividualHuman* NodeTB::addNewIndividual( float monte_carlo_weight, float initial_age, int gender, int initial_infection_count, float immparam, float riskparam, float mighet)
    {
        auto tempind = NodeAirborne::addNewIndividual(monte_carlo_weight, initial_age, gender, initial_infection_count, immparam, riskparam, mighet);
        //dynamic_cast<IndividualHumanCoInfection*>(tempind)->RegisterInfectionIncidenceObserver( this );
        return tempind;
    }

    float NodeTB::GetIncidentCounter() const 
    {
        return incident_counter;
    }

    float NodeTB::GetMDRIncidentCounter() const 
    {
        return MDR_incident_counter;
    }

    float NodeTB::GetMDREvolvedIncidentCounter() const 
    {
        return MDR_evolved_incident_counter;
    }

    float NodeTB::GetMDRFastIncidentCounter() const 
    {
        return MDR_fast_incident_counter;
    }

    REGISTER_SERIALIZABLE(NodeTB);

    void NodeTB::serialize(IArchive& ar, NodeTB* obj)
    {
        NodeAirborne::serialize(ar, obj);
        NodeTB& node = *obj;
        ar.labelElement("incident_counter") & node.incident_counter;
        ar.labelElement("MDR_incident_counter") & node.MDR_incident_counter;
        ar.labelElement("MDR_evolved_incident_counter") & node.MDR_evolved_incident_counter;
        ar.labelElement("MDR_fast_incident_counter") & node.MDR_fast_incident_counter;
    }
}

