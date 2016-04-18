/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "NodeTB.h"
#include "TransmissionGroupsFactory.h" //for SetupIntranodeTransmission
#include "NodeEventContextHost.h" //for node level trigger
#include "IndividualTB.h"
#include "SimulationConfig.h"

static const char* _module = "NodeTB";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(NodeTB, NodeAirborne)
        HANDLE_INTERFACE(INodeTB)
    END_QUERY_INTERFACE_DERIVED(NodeTB, NodeAirborne)


    NodeTB::~NodeTB(void) { }

    NodeTB::NodeTB() : NodeAirborne() { }

    NodeTB::NodeTB(ISimulationContext *_parent_sim, suids::suid node_suid) : NodeAirborne(_parent_sim, node_suid) { }

    const SimulationConfig* NodeTB::params()
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    NodeTB *NodeTB::CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid)
    {
        NodeTB *newnode = _new_ NodeTB(_parent_sim, node_suid);
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

    IIndividualHuman* NodeTB::createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender, float above_poverty)
    {
        return IndividualHumanTB::CreateHuman(this, suid, monte_carlo_weight, initial_age, gender, above_poverty);
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
            event_context_host->TriggerNodeEventObservers(ih->GetEventContext(), IndividualEventTriggerType::TBActivationPresymptomatic);
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
                 event_context_host->TriggerNodeEventObservers(ih->GetEventContext(), IndividualEventTriggerType::TBActivationPostRelapse);
            }
            else
            {
                event_context_host->TriggerNodeEventObservers(ih->GetEventContext(), IndividualEventTriggerType::TBActivation);
            }
            break;

        //  Infection got treatment and is now pending relapse - trigger goes off if you are ON OR OFF DRUGS.
        case InfectionStateChange::ClearedPendingRelapse:
            event_context_host->TriggerNodeEventObservers(ih->GetEventContext(), IndividualEventTriggerType::TBPendingRelapse);
            break;

        // no other infection state change is connected to a trigger, no trigger goes off in this time step
        default:
            //do nothing
            break;
        }
    }

    //NOTE: This is directly copied from Node.cpp, with the only difference being the use of StrainAwareGroups instead of SimpleGroups
    void NodeTB::SetupIntranodeTransmission()
    {
        transmissionGroups = TransmissionGroupsFactory::CreateNodeGroups(TransmissionGroupType::StrainAwareGroups);
        RouteToContagionDecayMap_t decayMap;
        if( demographics.Contains( IP_KEY ) && params()->heterogeneous_intranode_transmission_enabled)
        {
            ValidateIntranodeTransmissionConfiguration();

            const NodeDemographics& properties = demographics[IP_KEY];
            for (int iProperty = 0; iProperty < properties.size(); iProperty++)
            {
                const NodeDemographics& property = properties[iProperty];
                if (property.Contains(TRANSMISSION_MATRIX_KEY))
                {
                    const NodeDemographics& transmissionMatrix = property[ TRANSMISSION_MATRIX_KEY ];
                    PropertyValueList_t valueList;
                    const NodeDemographics& scalingMatrixRows = transmissionMatrix[TRANSMISSION_DATA_KEY];
                    ScalingMatrix_t scalingMatrix;

                    string routeName = transmissionMatrix.Contains( ROUTE_KEY ) ? transmissionMatrix[ ROUTE_KEY ].AsString() : "contact";
                    std::transform(routeName.begin(), routeName.end(), routeName.begin(), ::tolower);
                    string propertyName = property[IP_NAME_KEY].AsString();

                    if (routeName != "contact")
                    {
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, std::string( "Found route " + routeName + ". For generic sims, routes other than 'contact' are not supported, use Environmental sims for 'environmental' decay.").c_str());
                    }
                    else
                    {
                        if (decayMap.find(routeName)==decayMap.end())
                        {
                            LOG_DEBUG_F("HINT: Adding route %s.\n", routeName.c_str());
                            decayMap[routeName] = 1.0f;
                            routes.push_back(routeName);
                        }
                    }

                    if( propertyName == _age_bins_key )
                    {
                        int valueCount = distribs[ _age_bins_key ].size();
                        int counter = 0;
                        for( const auto& entry : distribs[ _age_bins_key ])
                        {
                            valueList.push_back( entry.second );
                            MatrixRow_t matrixRow;
                            const NodeDemographics& scalingMatrixRow = scalingMatrixRows[counter++];
    
                            for (int iSink = 0; iSink < valueCount; iSink++) 
                            {
                                 matrixRow.push_back(float(scalingMatrixRow[iSink].AsDouble()));
                            }
                            scalingMatrix.push_back(matrixRow);
                        }
                    }
                    else
                    {
                        const NodeDemographics& propertyValues = property[ IP_VALUES_KEY ];
                        int valueCount = propertyValues.size();
                        for (int iValue = 0; iValue < valueCount; iValue++)
                        {
                            valueList.push_back(propertyValues[iValue].AsString());
                            MatrixRow_t matrixRow;
                            const NodeDemographics& scalingMatrixRow = scalingMatrixRows[iValue];
    
                            for (int iSink = 0; iSink < valueCount; iSink++) 
                            {
                                 matrixRow.push_back(float(scalingMatrixRow[iSink].AsDouble()));
                            }
                            scalingMatrix.push_back(matrixRow);
                        }
                    }

                    LOG_DEBUG_F("adding property [%s]:%s\n", propertyName.c_str(), routeName.c_str());
                    transmissionGroups->AddProperty(propertyName, valueList, scalingMatrix, routeName);
                }
                else //HINT is enabled, but no transmission matrix is detected
                {
                    string default_route("contact");
                    float default_rate = 1.0f;
                    if (decayMap.find(default_route)==decayMap.end())
                    {
                        LOG_DEBUG("HINT on with no transmission matrix: Adding route 'contact'.\n");
                        decayMap[default_route] = default_rate;
                        routes.push_back(default_route);
                    }
                }
            }

        }
        else //HINT is not enabled
        {
            LOG_DEBUG("Non-HINT: Adding route 'contact'.\n");
            decayMap[string("contact")] = 1.0f;
            routes.push_back(string("contact"));
        }

        int max_antigens = GET_CONFIGURABLE(SimulationConfig)->number_basestrains;
        int max_genomes = GET_CONFIGURABLE(SimulationConfig)->number_substrains;
        LOG_DEBUG_F("max_antigens %f, max_genomes %f", max_antigens, max_genomes);
        transmissionGroups->Build(decayMap, max_antigens, max_genomes); 
    }

    void NodeTB::resetNodeStateCounters(void)
    {
        NodeAirborne::resetNodeStateCounters();
        incident_counter = 0.0f;
        MDR_incident_counter = 0.0f;
        MDR_evolved_incident_counter = 0.0f;
        MDR_fast_incident_counter = 0.0f;
    }

    void NodeTB::notifyOnInfectionIncidence(
        IndividualHumanTB * pIncident 
        )
    {
//        std::cout << pIncident->GetSuid().data << " just recovered from an infection." << std::endl;
        incident_counter += pIncident->GetMonteCarloWeight();
    }

    void NodeTB::notifyOnInfectionMDRIncidence(
        IndividualHumanTB * pIncident 
        )
    {
        MDR_incident_counter += pIncident->GetMonteCarloWeight();

        if ( pIncident->IsEvolvedMDR() ) 
        {
            MDR_evolved_incident_counter += pIncident->GetMonteCarloWeight();
        }
        if ( pIncident->IsFastProgressor() && !pIncident->IsEvolvedMDR() ) 
        {
            MDR_fast_incident_counter += pIncident->GetMonteCarloWeight(); //fast and transmitted
        }
    }

    IIndividualHuman* NodeTB::addNewIndividual( float monte_carlo_weight, float initial_age, int gender, int initial_infection_count, float immparam, float riskparam, float mighet, float init_poverty)
    {
        auto tempind = NodeAirborne::addNewIndividual(monte_carlo_weight, initial_age, gender, initial_infection_count, immparam, riskparam, mighet, init_poverty);
        dynamic_cast<IndividualHumanTB*>(tempind)->RegisterInfectionIncidenceObserver( this );
        return tempind;
    }

    void NodeTB::processEmigratingIndividual( IIndividualHuman* individual )
    {
        dynamic_cast<IndividualHumanTB*>(individual)->UnRegisterAllObservers( this );
        NodeAirborne::processEmigratingIndividual( individual );
    }

    IIndividualHuman* NodeTB::processImmigratingIndividual( IIndividualHuman* individual )
    {
        individual = NodeAirborne::processImmigratingIndividual( individual );
        dynamic_cast<IndividualHumanTB*>(individual)->RegisterInfectionIncidenceObserver( this );
        return individual;
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

#endif // ENABLE_TB
