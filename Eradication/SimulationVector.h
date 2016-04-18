/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

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
    struct IVectorMigrationReporting ;

    class SimulationVector : public Simulation, public IVectorSimulationContext
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static SimulationVector *CreateSimulation();
        static SimulationVector *CreateSimulation(const ::Configuration *config);
        virtual ~SimulationVector();

        // IVectorSimulationContext methods
        virtual void  PostMigratingVector( const suids::suid& nodeSuid, VectorCohort* ind ) override;
        virtual float GetNodePopulation( const suids::suid& nodeSuid ) override;
        virtual float GetAvailableLarvalHabitat( const suids::suid& nodeSuid, const std::string& rSpeciesID ) override;

        // Allows correct type of community to be added by derived class Simulations
        virtual void addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory) override;
        virtual int  populateFromDemographics(const char* campaign_filename, const char* loadbalance_filename);

        // Creates reporters.  Specifies vector-species-specific reporting in addition to base reporting
        virtual void Reports_CreateBuiltIn() override;

        // INodeInfoFactory
        virtual INodeInfo* CreateNodeInfo() override;
        virtual INodeInfo* CreateNodeInfo( int rank, INodeContext* pNC ) override;

    protected:

        // holds a vector of migrating vectors for each node rank
        vector<vector<IVectorCohort*>> migratingVectorQueues;
        vector< IVectorMigrationReporting* > vector_migration_reports ;
        std::map<suids::suid,float> node_populations_map ;

        float drugdefaultcost;
        float vaccinedefaultcost;
        float housingmoddefaultcost[HOUSINGMOD_ARRAY_LENGTH];
        float awarenessdefaultcost[AWARENESS_ARRAY_LENGTH];
        float netdefaultcost[BEDNET_ARRAY_LENGTH];

        virtual void Initialize(const ::Configuration *config) override;

        SimulationVector();
        static bool ValidateConfiguration(const ::Configuration *config);

        virtual void resolveMigration();
        virtual void setupMigrationQueues();

    private:

        virtual ISimulationContext *GetContextPointer();
    };
}
