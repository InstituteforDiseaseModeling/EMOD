/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "SimulationVector.h"
#include "ReportVector.h"
#include "SpatialReportVector.h"
#include "VectorSpeciesReport.h"
#include "NodeVector.h"
#include "IndividualVector.h"
#include "Sugar.h"
#include "Vector.h"
#include "SimulationConfig.h"
#include "IVectorMigrationReporting.h"
#include "NodeInfoVector.h"
#include "BinaryArchiveWriter.h"
#include "BinaryArchiveReader.h"
#include "MpiDataExchanger.h"
#include "VectorParameters.h"

#include <chrono>
typedef std::chrono::high_resolution_clock _clock;

SETUP_LOGGING( "SimulationVector" )

namespace Kernel
{
    // QI stuff in case we want to use it more extensively
    BEGIN_QUERY_INTERFACE_DERIVED(SimulationVector, Simulation)
        HANDLE_INTERFACE(IVectorSimulationContext)
    END_QUERY_INTERFACE_DERIVED(SimulationVector, Simulation)

    SimulationVector::SimulationVector()
        : Kernel::Simulation()
        , vector_migration_reports()
        , node_populations_map()
    {
        LOG_DEBUG( "SimulationVector ctor\n" );
        reportClassCreator = ReportVector::CreateReport;
        spatialReportClassCreator = SpatialReportVector::CreateReport;
    }

    void SimulationVector::Initialize(const ::Configuration *config)
    {
        Simulation::Initialize(config);
        IndividualHumanVector::InitializeStaticsVector( config );

        for( auto report : reports )
        {
            IVectorMigrationReporting* pivmr = dynamic_cast<IVectorMigrationReporting*>(report);
            if( pivmr != nullptr )
            {
                vector_migration_reports.push_back( pivmr );
            }
        }
    }

    SimulationVector *SimulationVector::CreateSimulation()
    {
        return _new_ SimulationVector();
    }

    SimulationVector *SimulationVector::CreateSimulation(const ::Configuration *config)
    {
        SimulationVector *newsimulation = _new_ SimulationVector();
        if (newsimulation)
        {
            // This sequence is important: first
            // Creation-->Initialization-->Validation
            newsimulation->Initialize(config);
            if(!ValidateConfiguration(config))
            {
                delete newsimulation;
                /* newsimulation = nullptr; */
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "VECTOR_SIM requested with invalid configuration." );
            }
        }

        return newsimulation;
    }

    bool SimulationVector::ValidateConfiguration(const ::Configuration *config)
    {
        bool validConfiguration = Kernel::Simulation::ValidateConfiguration(config);

        if (GET_CONFIGURABLE(SimulationConfig)->heterogeneous_intranode_transmission_enabled)
        {
            throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Sim_Type", "VECTOR_SIM", "Enable_Heterogeneous_Intranode_Transmission", "1" );
        }

        return validConfiguration;
    }

    SimulationVector::~SimulationVector()
    {
        // Node deletion handled by parent
        // No need to do any deletion for the flags
    }

    INodeInfo* SimulationVector::CreateNodeInfo()
    {
        INodeInfo* pin = new NodeInfoVector();
        return pin ;
    }

    INodeInfo* SimulationVector::CreateNodeInfo( int rank, INodeContext* pNC )
    {
        INodeInfo* pin = new NodeInfoVector( rank, pNC );
        return pin ;
    }

    void SimulationVector::Reports_CreateBuiltIn()
    {
        // Do base-class behavior for creating one or more reporters
        Simulation::Reports_CreateBuiltIn();

        // If we're running a simulation with actual vectors, do VectorSpeciesReport as well
        if(!GET_CONFIGURABLE(SimulationConfig)->vector_params->vector_species_names.empty())
            reports.push_back(VectorSpeciesReport::CreateReport( GET_CONFIGURABLE(SimulationConfig)->vector_params->vector_species_names ));
        else
            LOG_INFO("Skipping VectorSpeciesReport; no vectors detected in simulation\n");
    }

    // called by demographic file Populate()
    void SimulationVector::addNewNodeFromDemographics( suids::suid node_suid,
                                                       NodeDemographicsFactory *nodedemographics_factory,
                                                       ClimateFactory *climate_factory,
                                                       bool white_list_enabled )
    {
        NodeVector *node = NodeVector::CreateNode(this, node_suid);
        addNode_internal( node, nodedemographics_factory, climate_factory, white_list_enabled );
        node_populations_map.insert( std::make_pair( node_suid, node->GetStatPop() ) );
    }

    void SimulationVector::resolveMigration()
    {
        Simulation::resolveMigration(); // Take care of the humans

        WithSelfFunc to_self_func = [this](int myRank) 
        { 
#ifndef _DEBUG  // Standard path
            // Don't bother to serialize locally
            for (auto iterator = migratingVectorQueues[myRank].rbegin(); iterator != migratingVectorQueues[myRank].rend(); ++iterator)
            {
                // map.at() is faster than map[] since it doesn't optionally create an entry
                auto vector = *iterator;
                IMigrate* emigre = vector->GetIMigrate();
                emigre->ImmigrateTo( nodes.at(emigre->GetMigrationDestination()) );
            }
#else           // Test serialization even on single core.
            auto writer = new BinaryArchiveWriter();
            (*static_cast<IArchive*>(writer)) & migratingVectorQueues[myRank];
            for (auto& individual : migratingVectorQueues[myRank])
                individual->Recycle();
            migratingVectorQueues[myRank].clear();

            //if ( EnvPtr->Log->CheckLogLevel(Logger::VALIDATION, _module) ) {
            //    _write_json( int(currentTime.time), EnvPtr->MPI.Rank, myRank, "vect", static_cast<IArchive*>(writer)->GetBuffer(), static_cast<IArchive*>(writer)->GetBufferSize() );
            //}

            const char* buffer = static_cast<IArchive*>(writer)->GetBuffer();
            auto reader = new BinaryArchiveReader(buffer, static_cast<IArchive*>(writer)->GetBufferSize());
            (*static_cast<IArchive*>(reader)) & migratingVectorQueues[myRank];
            for (auto individual : migratingVectorQueues[myRank])
            {
                IMigrate* immigrant = individual->GetIMigrate();
                immigrant->ImmigrateTo( nodes[immigrant->GetMigrationDestination()]);
            }
            delete reader;
            delete writer;
#endif
        };

        SendToOthersFunc to_others_func = [this](IArchive* writer, int toRank)
        {
            *writer & migratingVectorQueues[toRank];
            for (auto& vector : migratingVectorQueues[toRank])
                vector->Recycle();  // delete vector
        };

        ClearDataFunc clear_data_func = [this](int rank)
        {
            migratingVectorQueues[rank].clear();
        };

        ReceiveFromOthersFunc from_others_func = [this](IArchive* reader, int fromRank)
        {
            *reader & migratingVectorQueues[fromRank];
            for (auto vector : migratingVectorQueues[fromRank])
            {
                IMigrate* immigrant = vector->GetIMigrate();
                immigrant->ImmigrateTo( nodes[immigrant->GetMigrationDestination()] );
            }
        };

        MpiDataExchanger exchanger( "VectorMigration", to_self_func, to_others_func, from_others_func, clear_data_func );
        exchanger.ExchangeData( this->currentTime );
    }

    int SimulationVector::populateFromDemographics( const char* campaign_filename, const char* loadbalance_filename )
    {
        int num_nodes = Simulation::populateFromDemographics( campaign_filename, loadbalance_filename );

        uint64_t total_vector_population = 0;
        for( auto node_entry : nodes )
        {
            node_populations_map[ node_entry.first ] = node_entry.second->GetStatPop() ;

            NodeVector* pnv = static_cast<NodeVector*>( node_entry.second );

            for( auto vp : pnv->GetVectorPopulationReporting() )
            {
                total_vector_population += uint64_t(vp->getAdultCount());
            }
        }

        if( total_vector_population == 0 )
        {
            LOG_WARN_F("!!!! NO VECTORS !!!!!  There are %d nodes on this core and zero vectors.",nodes.size());
        }

        return num_nodes ;
    }

    void SimulationVector::PostMigratingVector( const suids::suid& nodeSuid, IVectorCohort* ind )
    {
        for( auto report : vector_migration_reports )
        {
            report->LogVectorMigration( this, currentTime.time, nodeSuid, ind );
        }

        // put in queue by species and node rank
        auto& suid = ind->GetIMigrate()->GetMigrationDestination();
        auto rank = nodeRankMap.GetRankFromNodeSuid(suid);
        migratingVectorQueues.at(rank).push_back(ind);
    }

    float SimulationVector::GetNodePopulation( const suids::suid& nodeSuid )
    {
        return nodeRankMap.GetNodeInfo( nodeSuid ).GetPopulation() ;
    }

    float SimulationVector::GetAvailableLarvalHabitat( const suids::suid& nodeSuid, const std::string& rSpeciesID )
    {
        return ((NodeInfoVector&)nodeRankMap.GetNodeInfo( nodeSuid )).GetAvailableLarvalHabitat( rSpeciesID );
    }

    void SimulationVector::setupMigrationQueues()
    {
        Simulation::setupMigrationQueues();
        migratingVectorQueues.resize(EnvPtr->MPI.NumTasks);
    }

    ISimulationContext *SimulationVector::GetContextPointer()
    {
        return dynamic_cast<ISimulationContext*>(this);
    }

    REGISTER_SERIALIZABLE(SimulationVector);

    void SimulationVector::serialize(IArchive& ar, SimulationVector* obj)
    {
        Simulation::serialize( ar, obj );
        SimulationVector& sim = *obj;

//        ar.labelElement("migratingVectorQueues") & sim.migratingVectorQueues;         // no reason to keep track of migrating vectors "in-flight" :)
//        ar.labelElement("vector_migration_reports") & sim.vector_migration_reports;
//        ar.labelElement("node_populations_map") & sim.node_populations_map;           // should be reconstituted in populateFromDemographics()
    }
}
