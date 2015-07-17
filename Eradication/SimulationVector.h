/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <map>

#include "BoostLibWrapper.h"

#include "Simulation.h"
#include "VectorContexts.h"
#include "VectorCohort.h"
#include "VectorCohortIndividual.h" // for vector migration
#include "VectorPopulation.h"
#include "IndividualVector.h"
#include "Sugar.h"
#include "Common.h"
#include "VectorEnums.h"

#ifndef BEDNET_ARRAY_LENGTH
#define BEDNET_ARRAY_LENGTH 768
#endif

#ifndef HOUSINGMOD_ARRAY_LENGTH
#define HOUSINGMOD_ARRAY_LENGTH 12
#endif

#ifndef AWARENESS_ARRAY_LENGTH
#define AWARENESS_ARRAY_LENGTH 16
#endif

namespace Kernel
{
    class SimulationVector : public Simulation, public IVectorSimulationContext
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static SimulationVector *CreateSimulation();
        static SimulationVector *CreateSimulation(const ::Configuration *config);
        virtual ~SimulationVector();

        // IVectorSimulationContext methods
        virtual void  PostMigratingVector(VectorCohort* ind);

        // Allows correct type of community to be added by derived class Simulations
        virtual void addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory);

        // Creates reporters.  Specifies vector-species-specific reporting in addition to base reporting
        virtual void Reports_CreateBuiltIn();

    protected:

        // holds a vector of migrating vectors for each node rank
        vector< vector< IMigrate* > > migratingVectorQueues;

        float drugdefaultcost;
        float vaccinedefaultcost;
        float housingmoddefaultcost[HOUSINGMOD_ARRAY_LENGTH];
        float awarenessdefaultcost[AWARENESS_ARRAY_LENGTH];
        float netdefaultcost[BEDNET_ARRAY_LENGTH];

        void Initialize(const ::Configuration *config);

        SimulationVector();
        static bool ValidateConfiguration(const ::Configuration *config);

        virtual void resolveMigration();
        virtual void setupMigrationQueues();

    private:

#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, SimulationVector& sim, const unsigned int  file_version );
#endif

        virtual ISimulationContext *GetContextPointer();

        TypedPrivateMigrationQueueStorage<Kernel::IndividualHumanVector>  typed_migration_queue_storage;

    protected:
        TypedPrivateMigrationQueueStorage<Kernel::VectorCohortIndividual> typed_vector_migration_queue_storage; // inherited by SimulationMalaria
    };
}

#ifndef WIN32
DECLARE_VIRTUAL_BASE_OF(Kernel::Simulation, Kernel::SimulationVector)
#endif
