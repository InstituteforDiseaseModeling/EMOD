/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifndef DISABLE_AIRBORNE

#include "SimulationAirborne.h"

#include "InfectionAirborne.h"
#include "NodeAirborne.h"
#include "SusceptibilityAirborne.h"
#include "SimulationConfig.h"

SETUP_LOGGING( "SimulationAirborne" )

namespace Kernel
{
    SimulationAirborne::~SimulationAirborne(void) { }
    SimulationAirborne::SimulationAirborne() { }

    SimulationAirborne *SimulationAirborne::CreateSimulation()
    {
        SimulationAirborne *newsimulation = _new_ SimulationAirborne();
        newsimulation->Initialize();

        return newsimulation;
    }

    SimulationAirborne *SimulationAirborne::CreateSimulation(const ::Configuration *config)
    {
        SimulationAirborne *newsimulation = nullptr;

        newsimulation = _new_ SimulationAirborne();
        if (newsimulation)
        {
            // This sequence is important: first
            // Creation-->Initialization-->Validation
            newsimulation->Initialize(config);
            if(!ValidateConfiguration(config))
            {
                delete newsimulation;
                newsimulation = nullptr;
            }
        }

        return newsimulation;
    }

    bool SimulationAirborne::ValidateConfiguration(const ::Configuration *config)
    {
        if (!Simulation::ValidateConfiguration(config))
            return false;

        // TODO: any disease-specific validation goes here.
        // Warning: static climate parameters are not configured until after this function is called

        return true;
    }

    void SimulationAirborne::addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                         suids::suid node_suid,
                                                         NodeDemographicsFactory *nodedemographics_factory, 
                                                         ClimateFactory *climate_factory, 
                                                         bool white_list_enabled )
    {
        NodeAirborne *node = NodeAirborne::CreateNode(this, externalNodeId, node_suid);
        addNode_internal( node, nodedemographics_factory, climate_factory, white_list_enabled );
    }

    REGISTER_SERIALIZABLE(SimulationAirborne);

    void SimulationAirborne::serialize(IArchive& ar, SimulationAirborne* obj)
    {
        Simulation::serialize(ar, obj);
        // Nothing to do here
    }
}

#endif // DISABLE_AIRBORNE
