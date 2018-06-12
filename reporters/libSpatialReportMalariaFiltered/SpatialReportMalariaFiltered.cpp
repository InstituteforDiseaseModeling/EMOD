/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "SpatialReportMalariaFiltered.h"
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
SETUP_LOGGING( "SpatialReportMalariaFiltered" ) // <<< Name of this file

namespace Kernel
{
    // You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
    // "*" can be used if it applies to all simulation types.
    static const char * _sim_types[] = { "MALARIA_SIM", nullptr };// <<< Types of simulation the report is to be used with

    report_instantiator_function_t rif = []()
    {
        return (Kernel::IReport*)(new SpatialReportMalariaFiltered()); // <<< Report to create
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
            GetEModuleVersion( char* sVer, const Environment * pEnv )
        {
            return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
        }

        DTK_DLLEXPORT void __cdecl
            GetSupportedSimTypes( char* simTypes[] )
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
    // --- SpatialReportMalariaFiltered Methods
    // ----------------------------------------

#define DEFAULT_NAME ("SpatialReportMalariaFiltered.json")

    SpatialReportMalariaFiltered::SpatialReportMalariaFiltered()
        : SpatialReportMalaria()
        , m_NodesToInclude()
        , m_StartDay( 0.0 )
        , m_EndDay( FLT_MAX )
        , m_ReportingInterval(1.0)
        , m_IntervalTimer(1.0)
        , m_TimeElapsed(0.0)
        , m_Dt(1.0)
        , m_IsValidDay(false)
        , m_FilteredChannelDataMap()
    {
        report_name = DEFAULT_NAME;
    }

    SpatialReportMalariaFiltered::~SpatialReportMalariaFiltered()
    {
    }

    bool SpatialReportMalariaFiltered::Configure( const Configuration * inputJson )
    {
        std::vector<int> valid_external_node_id_list;

        initConfigTypeMap( "Node_IDs_Of_Interest", &valid_external_node_id_list, "Data will be collected for the nodes in tis list." );
        initConfigTypeMap( "Start_Day",            &m_StartDay,                  "Day to start collecting data",                        0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "End_Day",              &m_EndDay,                    "Day to stop collecting data",                         0.0f, FLT_MAX, FLT_MAX );
        initConfigTypeMap( "Reporting_Interval",   &m_ReportingInterval,         "Number of days to collect data and average over.",    0.0f, FLT_MAX, 1.0f );
        initConfigTypeMap( "Report_File_Name",     &report_name,                 "Name of the file to be written",                      DEFAULT_NAME );

        bool ret = SpatialReportMalaria::Configure( inputJson ); // picks up Spatial_Output_Channels from SpatialReport

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
            m_IntervalTimer = m_ReportingInterval;
        }
        return ret;
    }

    void SpatialReportMalariaFiltered::Initialize( unsigned int nrmSize )
    {
        // ----------------------------------------------------------------------------------
        // --- We only want to normalize on the number of nodes included in the report.
        // --- If m_NodesToInclude.size() == 0, then we are doing all nodes in the simulation
        // ----------------------------------------------------------------------------------
        _nrmSize = nrmSize;
        if( m_NodesToInclude.size() > 0 )
        {
            _nrmSize = m_NodesToInclude.size();
        }
        release_assert( _nrmSize );

        SpatialReportMalaria::Initialize( _nrmSize );

        // Initialize filtered map
        std::vector<std::string> channel_names = channelDataMap.GetChannelNames();
        for( auto name : channel_names )
        {
            m_FilteredChannelDataMap.IncreaseChannelLength( name, _nrmSize );
        }
    }

    void SpatialReportMalariaFiltered::UpdateEventRegistration( float currentTime,
        float dt,
        std::vector<INodeEventContext*>& rNodeEventContextList )
    {
        m_IsValidDay = (m_StartDay <= currentTime) && (currentTime <= m_EndDay);
        m_Dt = dt;
    }

    void SpatialReportMalariaFiltered::BeginTimestep()
    {
        if( m_IsValidDay && (m_IntervalTimer <= 0.0) )
        {
            m_IntervalTimer += m_ReportingInterval;
            m_TimeElapsed = 0.0;
        }
        SpatialReportMalaria::BeginTimestep();
    }

    void SpatialReportMalariaFiltered::LogNodeData( INodeContext* pNC )
    {
        if( m_IsValidDay && IsValidNode( pNC->GetExternalID() ) )
        {
            SpatialReportMalaria::LogNodeData( pNC );
        }
    }

    void SpatialReportMalariaFiltered::LogIndividualData( IIndividualHuman* individual )
    {
        if( m_IsValidDay && IsValidNode( individual->GetParent()->GetExternalID() ) )
        {
            SpatialReportMalaria::LogIndividualData( individual );
        }
    }

    bool SpatialReportMalariaFiltered::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return m_IsValidDay;
    }

    void SpatialReportMalariaFiltered::EndTimestep( float currentTime, float dt )
    {
        if( m_IsValidDay )
        {
            SpatialReportMalaria::EndTimestep( currentTime, dt );
        }
    }

    bool SpatialReportMalariaFiltered::IsValidNode( uint32_t externalNodeID ) const
    {
        bool valid = (m_NodesToInclude.size() == 0) || (m_NodesToInclude.count( externalNodeID ) > 0);
        return valid;
    }

    void SpatialReportMalariaFiltered::WriteHeaderParameters( std::ofstream* file )
    {
        SpatialReportMalaria::WriteHeaderParameters( file );
        file->write( (char*)&m_StartDay, sizeof( float ) );
        file->write( (char*)&m_ReportingInterval, sizeof( float ) );
    }

    void SpatialReportMalariaFiltered::Reduce()
    {
        if( m_IsValidDay )
        {
            m_IntervalTimer -= m_Dt;
            m_TimeElapsed += m_Dt;
            SpatialReportMalaria::Reduce();
        }
    }

    void SpatialReportMalariaFiltered::postProcessAccumulatedData()
    {
        if( m_IsValidDay )
        {
            SpatialReportMalaria::postProcessAccumulatedData();

            // ------------------------------------------------------------------------------
            // --- Accumulate the data collected this time step into a second data map.
            // --- This will allow us to average counts as well as the averages/percentages.
            // ------------------------------------------------------------------------------
            std::vector<std::string> channel_names = m_FilteredChannelDataMap.GetChannelNames();
            for( auto name : channel_names )
            {
                // ------------------------------------------------------------------------------------
                // --- Skip the NodeID channel because it contains identifiers and not calculated data 
                // ------------------------------------------------------------------------------------
                if( name == "NodeID" ) continue;

                const ChannelDataMap::channel_data_t& r_data = channelDataMap.GetChannel( name );
                for( int i = 0 ; i < r_data.size() ; ++i )
                {
                    m_FilteredChannelDataMap.Accumulate( name, i, r_data[i] );
                }
            }

            if( m_IntervalTimer <= 0.0 )
            {
                for( auto name : channel_names )
                {
                    if( name == "NodeID" ) continue;

                    // ----------------------------------
                    // --- Compute average value per day
                    // ----------------------------------
                    m_FilteredChannelDataMap.normalizeChannel( name, m_TimeElapsed );
                }
            }
        }
    }

    void SpatialReportMalariaFiltered::WriteData( ChannelDataMap& rChannelDataMap )
    {
        if( m_IsValidDay && (m_IntervalTimer <= 0.0) )
        {
            SpatialReportMalaria::WriteData( m_FilteredChannelDataMap );

            m_FilteredChannelDataMap.ClearData();
            m_FilteredChannelDataMap.IncreaseChannelLength( _nrmSize );
        }
    }

    void SpatialReportMalariaFiltered::ClearData()
    {
        if( m_IsValidDay )
        {
            SpatialReportMalaria::ClearData();
        }
    }
}