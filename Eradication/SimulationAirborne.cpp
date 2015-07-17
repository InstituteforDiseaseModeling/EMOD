/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "SimulationAirborne.h"

#include "InfectionAirborne.h"
#include "NodeAirborne.h"
#include "SusceptibilityAirborne.h"
#include "SimulationConfig.h"

static const char * _module = "SimulationAirborne";

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
        SimulationAirborne *newsimulation = NULL;

        newsimulation = _new_ SimulationAirborne();
        if (newsimulation)
        {
            // This sequence is important: first
            // Creation-->Initialization-->Validation
            newsimulation->Initialize(config);
            if(!ValidateConfiguration(config))
            {
                delete newsimulation;
                newsimulation = NULL;
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

    void SimulationAirborne::addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory)
    {
        NodeAirborne *node = NodeAirborne::CreateNode(this, node_suid);
        addNode_internal(node, nodedemographics_factory, climate_factory);
    }

    void SimulationAirborne::resolveMigration()
    {
        resolveMigrationInternal( typed_migration_queue_storage, migratingIndividualQueues );
    }

}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::SimulationAirborne)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, SimulationAirborne &sim, const unsigned int  file_version )
    {
        // Register derived types
        ar.template register_type<NodeAirborne>();
        ar.template register_type<NodeAirborneFlags>();

        // Serialize base class
        ar & boost::serialization::base_object<Simulation>(sim);
    }
}
#endif

#endif // ENABLE_TB
