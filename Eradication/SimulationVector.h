/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include "BoostLibWrapper.h"
#include "Simulation.h"
#include "VectorContexts.h"
#include "IVectorCohort.h"


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
        virtual void  PostMigratingVector( const suids::suid& nodeSuid, IVectorCohort* ind ) override;
        virtual float GetNodePopulation( const suids::suid& nodeSuid ) override;
        virtual float GetAvailableLarvalHabitat( const suids::suid& nodeSuid, const std::string& rSpeciesID ) override;

        // Allows correct type of community to be added by derived class Simulations
        virtual void addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                 suids::suid node_suid,
                                                 NodeDemographicsFactory *nodedemographics_factory,
                                                 ClimateFactory *climate_factory,
                                                 bool white_list_enabled ) override;
        virtual int  populateFromDemographics(const char* campaign_filename, const char* loadbalance_filename) override;

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

        virtual void Initialize(const ::Configuration *config) override;

        SimulationVector();
        static bool ValidateConfiguration(const ::Configuration *config);

        virtual void resolveMigration() override;
        virtual void setupMigrationQueues() override;

        DECLARE_SERIALIZABLE(SimulationVector);

    private:

        virtual ISimulationContext *GetContextPointer() override;
    };
}
