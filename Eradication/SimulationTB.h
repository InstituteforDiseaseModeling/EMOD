/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "SimulationAirborne.h"
#include "NodeTB.h" // for serialization forward reg ONLY
#include "SusceptibilityTB.h" // for serialization forward reg ONLY
#include "InfectionTB.h" // for serialization forward reg ONLY
#include "IndividualTB.h"

namespace Kernel
{
    class SimulationTB : public SimulationAirborne
    {
    public:
        static   SimulationTB *CreateSimulation();
        static   SimulationTB *CreateSimulation(const ::Configuration *config);
        virtual void Initialize( const ::Configuration *config );
        virtual ~SimulationTB(void);

    protected:

        SimulationTB();

        static bool ValidateConfiguration(const ::Configuration *config);

        // Allows correct type of Node to be added by classes derived from Simulation
        virtual void addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory);

        virtual void resolveMigration();

    private:

#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, SimulationTB &sim, const unsigned int  file_version );
#endif
        TypedPrivateMigrationQueueStorage<IndividualHumanTB> typed_migration_queue_storage;
    };
}

#ifndef WIN32
DECLARE_VIRTUAL_BASE_OF(Kernel::SimulationAirborne, Kernel::SimulationTB)
#endif
