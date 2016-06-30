/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "SimulationSTI.h"

#include "InfectionSTI.h"
#include "NodeSTI.h"
#include "ReportSTI.h"
#include "SusceptibilitySTI.h"
#include "SimulationConfig.h"
#include "StiObjectFactory.h"
#include "NodeInfoSTI.h"

static const char * _module = "SimulationSTI";

static const float DEFAULT_BASE_YEAR = 2015.0f ;

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(SimulationSTI,SimulationSTI)

    BEGIN_QUERY_INTERFACE_DERIVED(SimulationSTI, Simulation)
        HANDLE_INTERFACE(IIdGeneratorSTI)
        HANDLE_INTERFACE(ISTISimulationContext)
    END_QUERY_INTERFACE_DERIVED(SimulationSTI, Simulation)

    float SimulationSTI::base_year = 0.0f;

    SimulationSTI::SimulationSTI()
        : relationshipSuidGenerator(EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks)
        , report_relationship_start(false)
        , report_relationship_end(false)
        , report_transmission(false)
        , report_relationship_consummated(false)
    {
        initConfigTypeMap( "Report_Relationship_Start",     &report_relationship_start,      Report_Relationship_Start_DESC_TEXT, false);
        initConfigTypeMap( "Report_Relationship_End",       &report_relationship_end,        Report_Relationship_End_DESC_TEXT,   false);
        initConfigTypeMap( "Report_Transmission",           &report_transmission,            Report_Transmission_DESC_TEXT,      false);
        initConfigTypeMap( "Report_Coital_Acts",            &report_relationship_consummated,Report_Coital_Acts_DESC_TEXT,         false);

        reportClassCreator = ReportSTI::CreateReport;
    }

    SimulationSTI::~SimulationSTI(void)
    {
    }

    SimulationSTI *SimulationSTI::CreateSimulation()
    {
        SimulationSTI *newsimulation = _new_ SimulationSTI();
        newsimulation->Initialize();

        return newsimulation;
    }

    SimulationSTI *SimulationSTI::CreateSimulation(const ::Configuration *config)
    {
        SimulationSTI *newsimulation = nullptr;

        newsimulation = _new_ SimulationSTI();
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

    void
    SimulationSTI::Initialize()
    {
        Simulation::Initialize();
    }

    void
    SimulationSTI::Initialize(
        const ::Configuration *config
    )
    {
        Simulation::Initialize(config);
        IndividualHumanSTI::InitializeStaticsSTI( config );
    }

    bool
    SimulationSTI::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Base_Year",  &base_year, Base_Year_DESC_TEXT, MIN_YEAR, MAX_YEAR, DEFAULT_BASE_YEAR );

        bool ret = Simulation::Configure( inputJson );
        if( ret )
        {
            LOG_INFO_F("Setting Base_Year to %f\n", base_year );
            currentTime.setBaseYear( base_year );
        }

        return ret;
    }

    void
    SimulationSTI::Reports_CreateBuiltIn()
    {
        Simulation::Reports_CreateBuiltIn();

        // ---------------------------------------------------------------------------
        // --- We check the simulation type here because this method could be called
        // --- from a subclass like SimulationHIV.  SimulationHIV will only be calling
        // --- this if the simulation type is HIV_SIM
        // ---------------------------------------------------------------------------
        if (report_relationship_start && (GET_CONFIGURABLE( SimulationConfig )->sim_type == SimType::STI_SIM))
        {
            LOG_INFO( "Using STI RelationshipStartReporter.\n" );
            reports.push_back(StiObjectFactory::CreateRelationshipStartReporter(this));
        }

        if (report_relationship_end)
        {
            LOG_INFO( "Using STI RelationshipEndReporter.\n" );
            reports.push_back(StiObjectFactory::CreateRelationshipEndReporter(this));
        }

        if (report_relationship_consummated)
        {
            LOG_INFO( "Using STI RelationshipConsummatedReporter.\n" );
            reports.push_back(StiObjectFactory::CreateRelationshipConsummatedReporter(this));
        }

        if (report_transmission && (GET_CONFIGURABLE( SimulationConfig )->sim_type == SimType::STI_SIM))
        {
            LOG_INFO( "Using STI TransmissionReporter.\n" );
            reports.push_back(StiObjectFactory::CreateTransmissionReporter(this));
        }

    }

    bool SimulationSTI::ValidateConfiguration(const ::Configuration *config)
    {
        if (!Simulation::ValidateConfiguration(config))
            return false;

        // TODO: any disease-specific validation goes here.
        // Warning: static climate parameters are not configured until after this function is called

        return true;
    }

    void SimulationSTI::addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory)
    {
        NodeSTI *node = NodeSTI::CreateNode(this, node_suid);
        addNode_internal(node, nodedemographics_factory, climate_factory);
    }

    suids::suid SimulationSTI::GetNextRelationshipSuid()
    {
        return relationshipSuidGenerator();
    }

    void SimulationSTI::AddTerminatedRelationship( const suids::suid& nodeSuid, const suids::suid& relId )
    {
        INodeInfo& r_ni = nodeRankMap.GetNodeInfo( nodeSuid );
        NodeInfoSTI* p_ni_sti = dynamic_cast<NodeInfoSTI*>(&r_ni);
        p_ni_sti->AddTerminatedRelationship( relId );
    }

    bool SimulationSTI::WasRelationshipTerminatedLastTimestep( const suids::suid& relId ) const
    {
        const NodeRankMap::RankMap_t& rank_map = nodeRankMap.GetRankMap();

        for( auto& entry : rank_map )
        {
            NodeInfoSTI* p_nis = dynamic_cast<NodeInfoSTI*>(entry.second);
            release_assert( p_nis );

            if( p_nis->WasRelationshipTerminatedLastTimestep( relId ) )
            {
                return true;
            }
        }
        return false;
    }

    INodeInfo* SimulationSTI::CreateNodeInfo()
    {
        INodeInfo* pin = new NodeInfoSTI();
        return pin ;
    }

    INodeInfo* SimulationSTI::CreateNodeInfo( int rank, INodeContext* pNC )
    {
        INodeInfo* pin = new NodeInfoSTI( rank, pNC );
        return pin ;
    }

}
