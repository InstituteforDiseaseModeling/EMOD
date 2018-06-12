/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReportVectorMigration.h"
#include "DllInterfaceHelper.h"
#include "FactorySupport.h" // for DTK_DLLEXPORT

#include "VectorCohortIndividual.h"
#include "IMigrate.h"
#include "Contexts.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportVectorMigration" ) // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "VECTOR_SIM", "MALARIA_SIM", nullptr };// <<< Types of simulation the report is to be used with

report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportVectorMigration()); // <<< Report to create
};

DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ------------------------------
// --- DLL Interface Methods
// ---
// --- The DTK will use these methods to establish communication with the DLL.
// ------------------------------

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

DTK_DLLEXPORT char* __cdecl
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
}

DTK_DLLEXPORT void __cdecl
GetSupportedSimTypes(char* simTypes[])
{
    DLL_HELPER.GetSupportedSimTypes( simTypes );
}

DTK_DLLEXPORT const char * __cdecl
GetType()
{
    return DLL_HELPER.GetType();
}

DTK_DLLEXPORT void __cdecl
GetReportInstantiator( Kernel::report_instantiator_function_t* pif )
{
    DLL_HELPER.GetReportInstantiator( pif );
}

#ifdef __cplusplus
}
#endif

// ----------------------------------------
// --- ReportVectorMigration Methods
// ----------------------------------------

    ReportVectorMigration::ReportVectorMigration()
        : BaseTextReport( "ReportVectorMigration.csv" )
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportVectorMigration::~ReportVectorMigration()
    {
    }

    std::string ReportVectorMigration::GetHeader() const
    {
        std::stringstream header ;
        header << "Time"           << ", "
               << "ID"             << ", "
               << "FromNodeID"     << ", "
               << "ToNodeID"       << ", "
               << "MigrationType"  << ", "
               << "Species"        << ", "
               << "Age"
               ;

        return header.str();
    }

    void ReportVectorMigration::LogVectorMigration( ISimulationContext* pSim, float currentTime, const suids::suid& nodeSuid, IVectorCohort* pvc )
    {
        if( (currentTime <= 365.0) || (372.0 < currentTime) )
        {
            return;
        }

        IMigrate * pim = NULL;
        if (s_OK != pvc->QueryInterface(GET_IID(IMigrate), (void**)&pim) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pvc", "IMigrate", "IVectorCohort");
        }

        //uint64_t vci_id = pivci->GetID();
        uint64_t vci_id = 0;
        int from_node_id = pSim->GetNodeExternalID( nodeSuid ) ;
        int to_node_id = pSim->GetNodeExternalID( pim->GetMigrationDestination() ) ;
        int mig_type = pim->GetMigrationType() ;
        VectorStateEnum::Enum state = pvc->GetState();
        std::string species = pvc->GetSpecies();
        float age = pvc->GetAge();

        std::string mig_type_str = "" ;
        if( mig_type == MigrationType::LOCAL_MIGRATION )
            mig_type_str = "local" ;
        else if( mig_type == MigrationType::AIR_MIGRATION )
            mig_type_str = "air" ;
        else if( mig_type == MigrationType::REGIONAL_MIGRATION )
            mig_type_str = "regional" ;
        else if( mig_type == MigrationType::SEA_MIGRATION )
            mig_type_str = "sea" ;
        else
            release_assert( false );

        GetOutputStream() << currentTime
                   << "," << vci_id 
                   << "," << from_node_id 
                   << "," << to_node_id 
                   << "," << mig_type_str
                   << "," << species 
                   << "," << age 
                   << endl;
    }
}