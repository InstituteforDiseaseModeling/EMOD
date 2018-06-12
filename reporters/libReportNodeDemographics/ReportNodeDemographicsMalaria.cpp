/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#include "stdafx.h"

#include "ReportNodeDemographicsMalaria.h"
#include "DllInterfaceHelper.h"
#include "FactorySupport.h"

#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "ReportUtilities.h"
#include "Properties.h"
#include "NodeProperties.h"
#include "SimulationConfig.h"
#include "MalariaParameters.h"
#include "IGenomeMarkers.h"
#include "StrainIdentity.h"
#include "MalariaContexts.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportNodeDemographicsMalaria" ) // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "MALARIA_SIM", nullptr };// <<< Types of simulation the report is to be used with

report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportNodeDemographicsMalaria()); // <<< Report to create
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
// --- ReportNodeDemographicsMalaria Methods
// ----------------------------------------

    ReportNodeDemographicsMalaria::ReportNodeDemographicsMalaria()
        : ReportNodeDemographics( "ReportNodeDemographicsMalaria.csv" )
        , m_GenomeMarkerColumns()
    {
    }

    ReportNodeDemographicsMalaria::~ReportNodeDemographicsMalaria()
    {
    }

    void ReportNodeDemographicsMalaria::Initialize( unsigned int nrmSize )
    {
        SimulationConfig* p_sim_config = GET_CONFIGURABLE( SimulationConfig );
        std::vector<std::pair<std::string, uint32_t>> marker_combos = p_sim_config->malaria_params->pGenomeMarkers->CreatePossibleCombinations();

        for( auto& combo : marker_combos )
        {
            m_GenomeMarkerColumns.push_back( ReportUtilitiesMalaria::GenomeMarkerColumn( combo.first, combo.second ) );
        }

        ReportNodeDemographics::Initialize( nrmSize );

        for( int g = 0; g < m_Data.size(); ++g )
        {
            for( int a = 0; a < m_Data[ g ].size(); ++a )
            {
                for( int i = 0; i < m_Data[ g ][ a ].size(); ++i )
                {
                    NodeDataMalaria* p_ndm = (NodeDataMalaria*)m_Data[ g ][ a ][ i ];
                    p_ndm->genome_marker_columns = m_GenomeMarkerColumns;
                }
            }
        }
    }

    std::string ReportNodeDemographicsMalaria::GetHeader() const
    {
        std::stringstream header ;

        header << ReportNodeDemographics::GetHeader();

        header << "," << "AvgParasiteDensity";
        header << "," << "AvgGametocyteDensity";
        header << "," << "NumInfections";

        for( auto& r_column : m_GenomeMarkerColumns )
        {
            header << "," << r_column.GetColumnName();
        }

        return header.str();
    }

    void ReportNodeDemographicsMalaria::LogIndividualData( IIndividualHuman* individual, NodeData* pNodeData ) 
    {
        if( individual->IsInfected() )
        {
            IMalariaHumanContext* p_human_malaria = nullptr;
            if( s_OK != individual->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&p_human_malaria ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IMalariaHumanContext", "IIndividualHuman" );
            }

            std::vector<std::pair<int,int>> all_strains = p_human_malaria->GetInfectingStrainIds();

            NodeDataMalaria* p_ndm = (NodeDataMalaria*)pNodeData;
            p_ndm->avg_parasite_density   += p_human_malaria->GetMalariaSusceptibilityContext()->get_parasite_density();
            p_ndm->avg_gametocyte_density += p_human_malaria->GetGametocyteDensity();
            p_ndm->num_infections     += all_strains.size();

            for( auto& r_column : p_ndm->genome_marker_columns )
            {
                for( auto& strain : all_strains )
                {
                    if( r_column.GetBitMask() == strain.second )
                    {
                        r_column.AddCount( 1 );
                    }
                }
            }
        }
    }

    NodeData* ReportNodeDemographicsMalaria::CreateNodeData()
    {
        return new NodeDataMalaria();
    }

    void ReportNodeDemographicsMalaria::WriteNodeData( const NodeData* pData )
    {
        ReportNodeDemographics::WriteNodeData( pData );
        NodeDataMalaria* p_ndm = (NodeDataMalaria*)pData;
        GetOutputStream() << "," << p_ndm->avg_parasite_density / float( p_ndm->num_people );
        GetOutputStream() << "," << p_ndm->avg_gametocyte_density / float( p_ndm->num_people );
        GetOutputStream() << "," << p_ndm->num_infections;
        for( auto& r_column : p_ndm->genome_marker_columns )
        {
            GetOutputStream() << "," << r_column.GetCount();
        }
    }

}