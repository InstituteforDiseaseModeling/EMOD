/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#pragma warning(disable:4996)

#ifdef ENABLE_PYTHON

#include <math.h>
#include <numeric> // for std::accumulate
#include <functional> // why not algorithm?
#include "Sugar.h"
#include "Exceptions.h"
#include "NodePy.h"
#include "IndividualPy.h"
#include "TransmissionGroupsFactory.h"
#include "SimulationConfig.h"
#include "PythonSupport.h"

using namespace Kernel;

SETUP_LOGGING( "NodePy" )

#define ENABLE_PYTHON_FEVER 1
namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(NodePy, Node)
        HANDLE_INTERFACE(INodePy)
    END_QUERY_INTERFACE_DERIVED(NodePy, Node)


    NodePy::NodePy() : Node() { }

    NodePy::NodePy(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
        : Node(_parent_sim, externalNodeId, node_suid)
    {
    }

    void NodePy::Initialize()
    {
        Node::Initialize();
    }

    bool NodePy::Configure(
        const Configuration* config
    )
    {
        return Node::Configure( config );
    }

    NodePy *NodePy::CreateNode(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
    {
        NodePy *newnode = _new_ NodePy(_parent_sim, externalNodeId, node_suid);
        newnode->Initialize();

        return newnode;
    }

    NodePy::~NodePy(void)
    {
    }

#if 0
#define ROUTE_NAME_ENVIRONMENTAL "environmental"
#define ROUTE_NAME_CONTACT       "contact"
    void NodePy::SetupIntranodeTransmission()
    {
        //transmissionGroups = TransmissionGroupsFactory::CreateNodeGroups( TransmissionGroupType::MultiRouteGroups );
        transmissionGroups = TransmissionGroupsFactory::CreateNodeGroups( TransmissionGroupType::StrainAwareGroups );
        LOG_DEBUG_F("Number of basestrains: %d\n", GET_CONFIGURABLE(SimulationConfig)->number_basestrains);

        if( demographics.Contains( IP_KEY ) && GET_CONFIGURABLE(SimulationConfig)->heterogeneous_intranode_transmission_enabled)
        {
            ValidateIntranodeTransmissionConfiguration();
            const NodeDemographics& properties = demographics[IP_KEY];
            for (int iProperty = 0; iProperty < properties.size(); iProperty++)
            {
                const NodeDemographics& property = properties[iProperty];
                if (property.Contains(TRANSMISSION_MATRIX_KEY))
                {
                    string propertyName = property[IP_NAME_KEY].AsString();
                    string routeName = property[TRANSMISSION_MATRIX_KEY][ROUTE_KEY].AsString();
                    std::transform(routeName.begin(), routeName.end(), routeName.begin(), ::tolower);
                    if (decayMap.find(routeName)==decayMap.end())
                    {
                        if (routeName == ROUTE_NAME_CONTACT )
                        {
                            decayMap[routeName] = 1.0f;
                            routes.push_back(routeName);
                        }
                        else if (routeName == ROUTE_NAME_ENVIRONMENTAL )
                        {
                            decayMap[routeName] = node_contagion_decay_fraction;
                            routes.push_back(routeName);
                        }
                        else
                        {
                            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, std::string( "Found route " + routeName + ". For Environmental/polio sims, routes other than 'contact' and 'environmental' are not supported.").c_str());
                        }
                        LOG_DEBUG_F("HINT: Adding route %s.\n", routeName.c_str());
                    }

                    PropertyValueList_t valueList;
                    const NodeDemographics& propertyValues = property[IP_VALUES_KEY];
                    int valueCount = propertyValues.size();

                    const NodeDemographics& scalingMatrixRows = property[TRANSMISSION_MATRIX_KEY][TRANSMISSION_DATA_KEY];
                    ScalingMatrix_t scalingMatrix;

                    for (int iValue = 0; iValue < valueCount; iValue++)
                    {
                        valueList.push_back(propertyValues[iValue].AsString());
                        MatrixRow_t matrixRow;
                        const NodeDemographics& scalingMatrixRow = scalingMatrixRows[iValue];

                        for (int iSink = 0; iSink < valueCount; iSink++)
                        {
                            matrixRow.push_back((float)scalingMatrixRow[iSink].AsDouble());
                        }

                        scalingMatrix.push_back(matrixRow);
                    }
                    LOG_DEBUG_F("adding property [%s]:%s\n", propertyName.c_str(), routeName.c_str());
                    transmissionGroups->AddProperty(propertyName, valueList, scalingMatrix);
                }
                else //HINT is enabled, but no transmission matrix is detected
                {
                    string default_route = string("environmental");
                    float default_fraction = node_contagion_decay_fraction;
                    if (decayMap.find(default_route)==decayMap.end())
                    {
                        LOG_DEBUG("HINT on with no transmission matrix: Adding route 'environmental'.\n");
                        decayMap[default_route] = default_fraction;
                        routes.push_back(default_route);
                    }
                }
            }
        }
        else
        {
            //default scenario with no HINT
            LOG_DEBUG("non-HINT: Adding route 'environmental' and 'contact'.\n");
            decayMap[string( ROUTE_NAME_ENVIRONMENTAL )] = node_contagion_decay_fraction;
            decayMap[string( ROUTE_NAME_CONTACT )] = 1.0;
            routes.push_back( ROUTE_NAME_ENVIRONMENTAL );
            routes.push_back(string( ROUTE_NAME_CONTACT ));
        }

        transmissionGroups->Build(decayMap, GET_CONFIGURABLE(SimulationConfig)->number_basestrains, GET_CONFIGURABLE(SimulationConfig)->number_substrains);
    }
#endif 

    void NodePy::resetNodeStateCounters(void)
    {
        // This is a chance to do a single call into PYTHON_FEVER?g at start of timestep
#ifdef ENABLE_PYTHON_FEVER
        static auto pFunc = PythonSupport::GetPyFunction( PythonSupport::SCRIPT_PYTHON_FEVER.c_str(), "start_timestep" );
        if( pFunc )
        {
            PyObject_CallObject( pFunc, nullptr );
        }
#endif

        Node::resetNodeStateCounters();
    }

    void NodePy::updateNodeStateCounters(IndividualHuman *ih)
    {
        float mc_weight                = float(ih->GetMonteCarloWeight());
        IIndividualHumanPy *tempind2 = NULL;
        if( ih->QueryInterface( GET_IID( IIndividualHumanPy ), (void**)&tempind2 ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "tempind2", "IndividualHumanPy", "IndividualHuman" );
        }

        Node::updateNodeStateCounters(ih);
    }


    void NodePy::finalizeNodeStateCounters(void)
    {
        Node::finalizeNodeStateCounters();
       
    }

    void NodePy::populateNewIndividualsFromDemographics(int count_new_individuals)
    {
        // Populate the initial population
        Node::populateNewIndividualsFromDemographics(count_new_individuals);
    }

    IndividualHuman *NodePy::createHuman(suids::suid suid, float monte_carlo_weight, float initial_age, int gender)
    {
        return IndividualHumanPy::CreateHuman(this, suid, monte_carlo_weight, initial_age, gender);
    }

    NodePyTest *
    NodePyTest::CreateNode(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
    {
        auto *newnode = _new_ NodePyTest(_parent_sim, externalNodeId, node_suid);
        newnode->Initialize();

        return newnode;
    }

    NodePyTest::NodePyTest(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
    {
        parent = _parent_sim;
        auto newPerson = configureAndAddNewIndividual(1.0F /*mc*/, 0 /*age*/, 0.0f /*prev*/, 0.5f /*gender*/); // N.B. temp_prevalence=0 without maternal_transmission flag
        for (auto pIndividual : individualHumans)
        {
             // Nothing to do at the moment.
        }
    }
}
#endif // ENABLE_PYTHON
