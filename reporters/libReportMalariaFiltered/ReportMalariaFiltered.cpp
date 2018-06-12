/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReportMalariaFiltered.h"
#include "DllInterfaceHelper.h"
#include "FactorySupport.h"

#include "NodeEventContext.h"
#include "Individual.h"
#include "VectorContexts.h"
#include "VectorPopulation.h"
#include "IMigrate.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportMalariaFiltered" ) // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "MALARIA_SIM", nullptr };// <<< Types of simulation the report is to be used with

report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportMalariaFiltered()); // <<< Report to create
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
// --- ReportMalariaFiltered Methods
// ----------------------------------------

#define DEFAULT_NAME ("ReportMalariaFiltered.json")

    ReportMalariaFiltered::ReportMalariaFiltered()
        : ReportMalaria()
        , m_NodesToInclude()
        , m_StartDay(0.0)
        , m_EndDay(FLT_MAX)
    {
        report_name = DEFAULT_NAME;
    }

    ReportMalariaFiltered::~ReportMalariaFiltered()
    {
    }

    bool ReportMalariaFiltered::Configure( const Configuration * inputJson )
    {
        std::vector<int> valid_external_node_id_list;

        initConfigTypeMap("Node_IDs_Of_Interest", &valid_external_node_id_list, "Data will be collected for the nodes in tis list.");
        initConfigTypeMap( "Start_Day", &m_StartDay, "Day to start collecting data", 0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "End_Day",   &m_EndDay,   "Day to stop collecting data",  0.0f, FLT_MAX, FLT_MAX );
        initConfigTypeMap( "Report_File_Name", &report_name, "Name of the file to be written", DEFAULT_NAME );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_StartDay >= m_EndDay )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Start_Day", m_StartDay, "End_Day", m_EndDay );
            }
            for( auto node_id : valid_external_node_id_list )
            {
                m_NodesToInclude.insert( std::make_pair( node_id, true ) );
            }
        }
        return ret;
    }

    void ReportMalariaFiltered::Initialize( unsigned int nrmSize )
    {
        ReportMalaria::Initialize( nrmSize );

        // ----------------------------------------------------------------------------------
        // --- We only want to normalize on the number of nodes included in the report.
        // --- If m_NodesToInclude.size() == 0, then we are doing all nodes in the simulation
        // ----------------------------------------------------------------------------------
        if( m_NodesToInclude.size() > 0 )
        {
            _nrmSize = m_NodesToInclude.size();
        }
        release_assert( _nrmSize );
    }

    void ReportMalariaFiltered::UpdateEventRegistration( float currentTime, 
                                                         float dt, 
                                                         std::vector<INodeEventContext*>& rNodeEventContextList )
    {
        m_IsValidDay = (m_StartDay <= currentTime) && (currentTime <= m_EndDay);
    }

    void ReportMalariaFiltered::BeginTimestep()
    {
        if( m_IsValidDay )
        {
            ReportMalaria::BeginTimestep();
        }
    }

    void ReportMalariaFiltered::LogNodeData( INodeContext* pNC )
    {
        if( m_IsValidDay && IsValidNode( pNC->GetExternalID() ) )
        {
            ReportMalaria::LogNodeData( pNC );
        }
    }

    void ReportMalariaFiltered::LogIndividualData( IIndividualHuman* individual ) 
    {
        if( m_IsValidDay && IsValidNode( individual->GetParent()->GetExternalID() ) )
        {
            ReportMalaria::LogIndividualData( individual );
        }
    }

    bool ReportMalariaFiltered::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return m_IsValidDay;
    }

    bool ReportMalariaFiltered::IsValidNode( uint32_t externalNodeID ) const
    {
        bool valid = (m_NodesToInclude.size() == 0) || (m_NodesToInclude.count( externalNodeID ) > 0);
        return valid;
    }
}