/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "Simulation.h"
#include "IndividualAirborne.h"
#include "Sugar.h" // for DECLARE_VIRTUAL_BASE

namespace Kernel
{
    class SimulationAirborne : public Simulation
    {
    public:
        virtual ~SimulationAirborne(void);
        static SimulationAirborne *CreateSimulation();
        static SimulationAirborne *CreateSimulation(const ::Configuration *config);
        SimulationAirborne();

    protected:

        static bool ValidateConfiguration(const ::Configuration *config);

        // Allows correct type of Node to be added by classes derived from Simulation
        virtual void addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory);

        virtual void resolveMigration();

    private:
#if USE_BOOST_SERIALIZATION
        template<class Archive>
        friend void serialize(Archive & ar, SimulationAirborne &sim, const unsigned int  file_version );
#endif
        TypedPrivateMigrationQueueStorage<IndividualHumanAirborne> typed_migration_queue_storage;
    };
}

DECLARE_VIRTUAL_BASE_OF(Kernel::Simulation, Kernel::SimulationAirborne)
