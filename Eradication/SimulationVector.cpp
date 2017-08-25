/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
#include "VectorCohortIndividual.h"
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
        , drugdefaultcost(1.0f)
        , vaccinedefaultcost(DEFAULT_VACCINE_COST)
    {
        LOG_DEBUG( "SimulationVector ctor\n" );

        // The following arrays hold the total interventions distributed by intervention and distribution detail. 
        // This allows different costing models to be applied to study the effect of different distribution modes
        // the overall way the array is divided up is a bit difficult to parse out of this quickly, but this section will be redone in the next iteration for easier customization
        for (int i = 0; i < BEDNET_ARRAY_LENGTH; i++)
        {
            //type of net
            if (i < BEDNET_ARRAY_LENGTH / 4)
                netdefaultcost[i] = DEFAULT_BARRIER_COST;
            else if (i < BEDNET_ARRAY_LENGTH / 2 && i >= BEDNET_ARRAY_LENGTH / 4)
                netdefaultcost[i] = DEFAULT_ITN_COST;
            else if (i < 3 * BEDNET_ARRAY_LENGTH / 4 && i >= BEDNET_ARRAY_LENGTH / 2)
                netdefaultcost[i] = DEFAULT_LLIN_COST;
            else
                netdefaultcost[i] = DEFAULT_RETREATMENT_COST;

            //cost to user/campaign
            // TODO Those magic numbers you see before you today, you will never see again, but will be removed in distribution refactor in August 2011
            if (int(i / 64) == 1 || int(i / 64) == 4 || int(i / 64) == 7 || int(i / 64) == 10)
                netdefaultcost[i] *= 0.75;
            else if (int(i / 64) == 2 || int(i / 64) == 5 || int(i / 64) == 8 || int(i / 64) == 11)
                netdefaultcost[i] *= 0.1f;

            //delivery type
            if (int(i / 16) % 4 == 0)
                netdefaultcost[i] += 0.50; //sentinel
            else if (int(i / 16) % 4 == 1)
                netdefaultcost[i] += 1.00; //catchup without other campaign to share costs
            else if (int(i / 16) % 4 == 2)
                netdefaultcost[i] += 2.00; //door-to-door
            else if (int(i / 16) % 4 == 3)
                netdefaultcost[i] += 4.00; //door-to-door with verification

            //urban vs rural
            if (int(i / 2) % 2 == 0)
                netdefaultcost[i] += 0.17f; //urban
            else
                netdefaultcost[i] += 0.33f;   //rural
        }

        for (int i = 0; i < HOUSINGMOD_ARRAY_LENGTH; i++)
        {
            if (i < 4)
                housingmoddefaultcost[i] = DEFAULT_IRS_COST;
            else if (i < 8 && i >= 4)
                housingmoddefaultcost[i] = DEFAULT_SCREENING_COST;
            else
                housingmoddefaultcost[i] = DEFAULT_IRS_COST + DEFAULT_SCREENING_COST;
            if (int(i / 2) % 2 == 0)
                netdefaultcost[i] += 0.50; //urban
            else
                netdefaultcost[i] += 1.00; //rural
        }

        for (int i = 0; i < AWARENESS_ARRAY_LENGTH; i++)
        {
            awarenessdefaultcost[i] = 1.0;
        }
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

        int total_vector_population = 0;
        for( auto node_entry : nodes )
        {
            node_populations_map[ node_entry.first ] = node_entry.second->GetStatPop() ;

            NodeVector* pnv = static_cast<NodeVector*>( node_entry.second );

            for( auto vp : pnv->GetVectorPopulationReporting() )
            {
                total_vector_population += vp->getAdultCount();
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

        // cast to VectorCohortIndividual
        // TBD: Get rid of cast, replace with QI. Not such a big deal at Simulation level
        VectorCohortIndividual* vci = static_cast<VectorCohortIndividual*>(ind);

        // put in queue by species and node rank
        auto& suid = vci->VectorCohortIndividual::GetMigrationDestination();
        auto rank = nodeRankMap.GetRankFromNodeSuid(suid);
        migratingVectorQueues.at(rank).push_back(vci);
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

        ar.labelElement("drugdefaultcost")    & sim.drugdefaultcost;
        ar.labelElement("vaccinedefaultcost") & sim.vaccinedefaultcost;
        ar.labelElement("housingmoddefaultcost"); ar.serialize( sim.housingmoddefaultcost, HOUSINGMOD_ARRAY_LENGTH );
        ar.labelElement("awarenessdefaultcost");  ar.serialize( sim.awarenessdefaultcost, AWARENESS_ARRAY_LENGTH );
        ar.labelElement("netdefaultcost");        ar.serialize( sim.netdefaultcost, BEDNET_ARRAY_LENGTH );
    }
}
