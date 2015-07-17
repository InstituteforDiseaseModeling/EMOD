/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "BoostLibWrapper.h"

#include "MalariaContexts.h"
#include "SimulationVector.h"
#include "IndividualMalaria.h"
#include "NodeMalaria.h" // for forward-registering serialization only, needed???
#include "InfectionMalaria.h" // for forward-registering serialization only, needed???

#include "Sugar.h"
#include "Common.h"

namespace Kernel
{
    class SimulationMalaria : public SimulationVector
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        SimulationMalaria();
        static SimulationMalaria *CreateSimulation();
        static SimulationMalaria *CreateSimulation(const ::Configuration *config);
        virtual ~SimulationMalaria();

        // Allows correct type of community to be added by derived class Simulations
        virtual void addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory);

    protected:
        static bool ValidateConfiguration(const ::Configuration *config);

        virtual void InitializeFlags(const ::Configuration *config);  // override in derived classes to instantiate correct flag classes
        virtual void resolveMigration();

    private:
        void Initialize(const ::Configuration *config);

#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, SimulationMalaria & sim, const unsigned int  file_version );
#endif

        virtual ISimulationContext *GetContextPointer();
        TypedPrivateMigrationQueueStorage<Kernel::IndividualHumanMalaria> typed_migration_queue_storage;
    };
}

#ifndef WIN32
DECLARE_VIRTUAL_BASE_OF(Kernel::SimulationVector, Kernel::SimulationMalaria)
#endif
