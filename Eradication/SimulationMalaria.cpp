/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "SimulationMalaria.h"
#include "NodeMalaria.h"
#include "ReportMalaria.h"
#include "BinnedReportMalaria.h"
#include "SpatialReportMalaria.h"

#pragma warning(disable : 4996)

static const char * _module = "SimulationMalaria";

#ifdef _MALARIA_DLL

// Note: _diseaseType has to match the Simulation_Type name in config.json
static std::string _diseaseType = "MALARIA_SIM";

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

//
// This is the interface function from the DTK.
//
namespace Kernel
{
    DTK_DLLEXPORT ISimulation* __cdecl
    CreateSimulation(
        const Environment * pEnv 
    )
    {
        Environment::setInstance(const_cast<Environment*>(pEnv));
        LOG_INFO("CreateSimulation called\n");
        return Kernel::SimulationMalaria::CreateSimulation( EnvPtr->Config );
    }
}

DTK_DLLEXPORT
const char * __cdecl
GetDiseaseType()
{
    LOG_INFO_F("GetDiseaseType called for %s\n", _module);
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
    ///// Simulation /////

    // QI stuff in case we want to use it more extensively
    BEGIN_QUERY_INTERFACE_DERIVED(SimulationMalaria, SimulationVector)
    END_QUERY_INTERFACE_DERIVED(SimulationMalaria, SimulationVector)

    SimulationMalaria::SimulationMalaria() : SimulationVector()
    {
        reportClassCreator = ReportMalaria::CreateReport;
        binnedReportClassCreator = BinnedReportMalaria::CreateReport;
        spatialReportClassCreator = SpatialReportMalaria::CreateReport;
    }

    void SimulationMalaria::Initialize(const ::Configuration *config)
    {
        SimulationVector::Initialize(config);
        IndividualHumanMalaria fakeHuman;
        LOG_INFO( "Calling Configure on fakeHumanMalaria\n" );
        fakeHuman.Configure( config );
    }

    SimulationMalaria *SimulationMalaria::CreateSimulation()
    {
        return _new_ SimulationMalaria();
    }

    SimulationMalaria *SimulationMalaria::CreateSimulation(const ::Configuration *config)
    {
        SimulationMalaria *newsimulation = _new_ SimulationMalaria();
        if (newsimulation)
        {
            // This sequence is important: first
            // Creation-->Initialization-->Validation
            newsimulation->Initialize(config);
            if(!ValidateConfiguration(config))
            {
                delete newsimulation;
                newsimulation = nullptr;
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Malaria simulations do not currently support heterogeneous intra-node transmission.");
            }
        }

        return newsimulation;
    }

    bool SimulationMalaria::ValidateConfiguration(const ::Configuration *config)
    {
        return Kernel::SimulationVector::ValidateConfiguration(config);
    }

    SimulationMalaria::~SimulationMalaria()
    {
        // Node deletion handled by ~Kernel::Simulation()
        // no need to delete flags
        //delete m_strain_identity_flags;
    }

    void SimulationMalaria::InitializeFlags(const ::Configuration *config)
    {
    }

    void SimulationMalaria::addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory)
    {
        NodeMalaria *node = NodeMalaria::CreateNode(GetContextPointer(), node_suid);
        addNode_internal(node, nodedemographics_factory, climate_factory);
    }

    ISimulationContext *
    SimulationMalaria::GetContextPointer() { return (ISimulationContext*)this; }
} // end namespace Kernel

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, SimulationMalaria &sim, const unsigned int  file_version )
    {
        ar & sim.m_strain_identity_flags;

        ar & boost::serialization::base_object<Kernel::SimulationVector>(sim);
    }
}
#endif