/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReportVectorStatsMalaria.h"
#include "DllInterfaceHelper.h"
#include "FactorySupport.h"

#include "NodeEventContext.h"
#include "Individual.h"
#include "VectorContexts.h"
#include "VectorPopulation.h"
#include "ReportUtilities.h"
#include "SimulationConfig.h"
#include "MalariaParameters.h"
#include "IGenomeMarkers.h"
#include "StrainIdentity.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportVectorStatsMalaria" ) // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "MALARIA_SIM", nullptr };// <<< Types of simulation the report is to be used with

report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportVectorStatsMalaria()); // <<< Report to create
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
// --- ReportVectorStatsMalaria Methods
// ----------------------------------------

    ReportVectorStatsMalaria::ReportVectorStatsMalaria()
        : ReportVectorStats( "ReportVectorStatsMalaria.csv" )
        , genome_marker_columns()
    {
    }

    ReportVectorStatsMalaria::~ReportVectorStatsMalaria()
    {
    }

    bool ReportVectorStatsMalaria::Configure( const Configuration * inputJson )
    {
        bool ret = ReportVectorStats::Configure( inputJson );

        if( ret )
        {
            SimulationConfig* p_sim_config = GET_CONFIGURABLE( SimulationConfig );
            std::vector<std::pair<std::string,uint32_t>> marker_combos = p_sim_config->malaria_params->pGenomeMarkers->CreatePossibleCombinations();

            for( auto& combo : marker_combos )
            {
                genome_marker_columns.push_back( ReportUtilitiesMalaria::GenomeMarkerColumn( combo.first, combo.second ) );
            }
        }
        return ret;
    }

    std::string ReportVectorStatsMalaria::GetHeader() const
    {
        std::stringstream header ;

        header << ReportVectorStats::GetHeader();

        for( auto& r_column : genome_marker_columns )
        {
            header << ", " << r_column.GetColumnName();
        }

        return header.str();
    }

    void ReportVectorStatsMalaria::ResetOtherCounters()
    {
        for( auto& r_column : genome_marker_columns )
        {
            r_column.ResetCount();
        }
    }

    void ReportVectorStatsMalaria::CollectOtherData( IVectorPopulationReporting* pIVPR )
    {
        StrainIdentity strain;
        for( auto& r_column : genome_marker_columns )
        {
            strain.SetGeneticID( r_column.GetBitMask() );
            uint32_t gm_infected = pIVPR->getInfectedCount( &strain );
            uint32_t gm_infectious = pIVPR->getInfectiousCount( &strain );
            r_column.AddCount( gm_infected + gm_infectious );
        }
    }

    void ReportVectorStatsMalaria::WriteOtherData()
    {
        for( auto& r_column : genome_marker_columns )
        {
            GetOutputStream() << "," << r_column.GetCount();
        }
    }
}