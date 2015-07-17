/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "SimulationSTI.h"

#include "InfectionSTI.h"
#include "NodeSTI.h"
#include "ReportSTI.h"
#include "SusceptibilitySTI.h"
#include "SimulationConfig.h"
#include "StiObjectFactory.h"

static const char * _module = "SimulationSTI";

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(SimulationSTI,SimulationSTI)

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
        SimulationSTI *newsimulation = NULL;

        newsimulation = _new_ SimulationSTI();
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

    void
    SimulationSTI::Initialize()
    {
    }

    void
    SimulationSTI::Initialize(
        const ::Configuration *config
    )
    {
        Simulation::Initialize(config);
        IndividualHumanSTIConfig fakeHumanSTIConfig;
        LOG_INFO( "Calling Configure on fakeHumanSTIConfig\n" );
        fakeHumanSTIConfig.Configure( config );
    }

    bool
    SimulationSTI::Configure(
        const Configuration * inputJson
    )
    {
        bool ret = Simulation::Configure( inputJson );
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

    void SimulationSTI::resolveMigration()
    {
        resolveMigrationInternal( typed_migration_queue_storage, migratingIndividualQueues );
    }

    suids::suid SimulationSTI::GetNextRelationshipSuid()
    {
        return relationshipSuidGenerator();
    }
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::SimulationSTI)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, SimulationSTI &sim, const unsigned int  file_version )
    {
        // Register derived types
        ar.template register_type<NodeSTI>();
        ar.template register_type<NodeSTIFlags>();

        // Serialize base class
        ar & boost::serialization::base_object<Simulation>(sim);
    }
}
#endif

