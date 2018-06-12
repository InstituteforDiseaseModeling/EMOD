/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#pragma warning(disable : 4996)

#ifdef ENABLE_PYTHON

#include "SimulationPy.h"
#include "NodePy.h"
#include "IndividualPy.h"
#include "InfectionPy.h"
#include "SusceptibilityPy.h"
#include "suids.hpp"
#include "ReportPy.h"
#include "BinnedReportPy.h"
#include "SpatialReportPy.h"
#include "ProgVersion.h"

SETUP_LOGGING( "SimulationPy" )

#ifdef _TYPHOID_DLL

// Note: _diseaseType has to match the Simulation_Type name in config.json
static std::string _diseaseType = "TYPHOID_SIM";

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

//
// This is the interface function from the DTK.
//
DTK_DLLEXPORT Kernel::ISimulation* __cdecl
CreateSimulation(
    const Environment * pEnv
)
{
    Environment::setInstance(const_cast<Environment*>(pEnv));
    LOG_INFO("CreateSimulation called for \n");
    return Kernel::SimulationPy::CreateSimulation( EnvPtr->Config );
}

DTK_DLLEXPORT
const char *
__cdecl
GetDiseaseType()
{
    LOG_INFO("GetDiseaseType called for \n");
    return _diseaseType.c_str();
}

DTK_DLLEXPORT
char* __cdecl
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    Environment::setInstance(const_cast<Environment*>(pEnv));
    ProgDllVersion pv;
    LOG_INFO_F("GetEModuleVersion called with ver=%s\n", pv.getVersion());
    if (sVer) strcpy(sVer, pv.getVersion());
    return sVer;
}

DTK_DLLEXPORT
const char* __cdecl
GetSchema()
{
    LOG_DEBUG_F("GetSchema called for %s: map has size %d\n", _module, Kernel::JsonConfigurable::get_registration_map().size() );
    
    json::Object configSchemaAll;
    std::ostringstream schemaJsonString;
    for (auto& entry : Kernel::JsonConfigurable::get_registration_map())
    {
        const std::string classname = entry.first;
        LOG_DEBUG_F( "classname = %s\n", classname.c_str() );
        json::QuickBuilder config_schema = ((*(entry.second))());
        configSchemaAll[classname] = config_schema;
    }

    json::Writer::Write( configSchemaAll, schemaJsonString );

    putenv( ( std::string( "GET_SCHEMA_RESULT=" ) + schemaJsonString.str().c_str() ).c_str() );
    return schemaJsonString.str().c_str();
}
#ifdef __cplusplus
}
#endif

#endif

namespace Kernel
{
    SimulationPy::SimulationPy() : Simulation()
    {
        reportClassCreator = ReportPy::CreateReport;
        binnedReportClassCreator = BinnedReportPy::CreateReport;
        spatialReportClassCreator = SpatialReportPy::CreateReport;
    }

    void SimulationPy::Initialize()
    {
        Simulation::Initialize();
    }

    void SimulationPy::Initialize(const ::Configuration *config)
    {
        Simulation::Initialize(config);
        IndividualHumanPy::InitializeStaticsPy( config );
    }

    SimulationPy *SimulationPy::CreateSimulation()
    {
        // Fail fast if Python isn't initialized
        if ( !PythonSupport::IsPythonInitialized() )
        {
            throw InitializationException(__FILE__, __LINE__, __FUNCTION__, "Failed to create Python simulation: Python not initialized. Make sure to provide a Python script path command line argument.\n");
        }

        SimulationPy *newsimulation = _new_ SimulationPy();
        newsimulation->Initialize();

        return newsimulation;
    }

    SimulationPy *SimulationPy::CreateSimulation(const ::Configuration *config)
    {
        // Fail fast if Python isn't initialized
        if ( !PythonSupport::IsPythonInitialized() )
        {
            throw InitializationException(__FILE__, __LINE__, __FUNCTION__, "Failed to create Python simulation: Python not initialized. Make sure to provide a Python script path command line argument.\n");
        }

       SimulationPy *newsimulation = NULL;
       
       newsimulation = _new_ SimulationPy();
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

    bool SimulationPy::ValidateConfiguration(const ::Configuration *config)
    {
        return Kernel::Simulation::ValidateConfiguration(config);
    }

    // called by demographic file Populate()
    void SimulationPy::addNewNodeFromDemographics( suids::suid node_suid,
                                                   NodeDemographicsFactory *nodedemographics_factory,
                                                   ClimateFactory *climate_factory,
                                                   bool white_list_enabled )
    {
        NodePy *node = NodePy::CreateNode(this, node_suid);

        addNode_internal( node, nodedemographics_factory, climate_factory, white_list_enabled );
    }

    void SimulationPy::InitializeFlags( const ::Configuration *config )
    {
    }


    void SimulationPy::resolveMigration()
    {
        //resolveMigrationInternal( typed_migration_queue_storage, migratingIndividualQueues );
    }

    void SimulationPy::Reports_CreateBuiltIn()
    {
        return Simulation::Reports_CreateBuiltIn();
    }
}

#endif // 
