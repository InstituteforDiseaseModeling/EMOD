
#include "stdafx.h"

#include "SpatialReportMalariaFiltered.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "Individual.h"
#include "VectorContexts.h"
#include "IMigrate.h"
#include "INodeContext.h"
#include "IdmDateTime.h"
#include "SimulationEventContext.h"

#ifdef _REPORT_DLL
#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"
#include "FactorySupport.h"
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "SpatialReportMalariaFiltered" ) // <<< Name of this file

namespace Kernel
{
#ifdef _REPORT_DLL

    // You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
    // "*" can be used if it applies to all simulation types.
    static const char * _sim_types[] = { "MALARIA_SIM", nullptr };// <<< Types of simulation the report is to be used with

    instantiator_function_t rif = []()
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

        DTK_DLLEXPORT char*
            GetEModuleVersion( char* sVer, const Environment * pEnv )
        {
            return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
        }

        DTK_DLLEXPORT void
            GetSupportedSimTypes( char* simTypes[] )
        {
            DLL_HELPER.GetSupportedSimTypes( simTypes );
        }

        DTK_DLLEXPORT const char *
            GetType()
        {
            return DLL_HELPER.GetType();
        }

        DTK_DLLEXPORT void
            GetReportInstantiator( Kernel::instantiator_function_t* pif )
        {
            DLL_HELPER.GetReportInstantiator( pif );
        }

#ifdef __cplusplus
    }
#endif
#endif //_REPORT_DLL

    // ----------------------------------------
    // --- SpatialReportMalariaFiltered Methods
    // ----------------------------------------

#define DEFAULT_NAME ("SpatialReportMalariaFiltered")

    BEGIN_QUERY_INTERFACE_DERIVED( SpatialReportMalariaFiltered, SpatialReportMalaria )
    END_QUERY_INTERFACE_DERIVED( SpatialReportMalariaFiltered, SpatialReportMalaria )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( SpatialReportMalariaFiltered )
#endif

    SpatialReportMalariaFiltered::SpatialReportMalariaFiltered()
        : SpatialReportMalaria()
        , m_ReportFilter( nullptr, "", false, true, true )
        , m_ReportingInterval(1.0)
        , m_IntervalTimer(1.0)
        , m_TimeElapsed(0.0)
        , m_Dt(1.0)
        , m_IsValidDay(false)
        , m_HasValidNode( false )
        , m_FilteredChannelDataMap()
    {
        initSimTypes( 1, "MALARIA_SIM" );
        report_name = DEFAULT_NAME;
    }

    SpatialReportMalariaFiltered::~SpatialReportMalariaFiltered()
    {
    }

    bool SpatialReportMalariaFiltered::Configure( const Configuration * inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        initConfigTypeMap( "Reporting_Interval", &m_ReportingInterval, SRMF_Reporting_Interval_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );

        bool ret = SpatialReportMalaria::Configure( inputJson ); // picks up Spatial_Output_Channels from SpatialReport

        if( ret && !JsonConfigurable::_dryrun )
        {
            m_ReportFilter.CheckParameters( inputJson );

            m_IntervalTimer = m_ReportingInterval;
        }
        return ret;
    }

    const char* SpatialReportMalariaFiltered::GetChannelsDependsOn() const
    {
        return nullptr;
    }

    void SpatialReportMalariaFiltered::Initialize( unsigned int nrmSize )
    {
        m_ReportFilter.Initialize();
    }

    void SpatialReportMalariaFiltered::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& nodeIds_demographics )
    {
        m_ReportFilter.CheckForValidNodeIDs( GetReportName(), nodeIds_demographics );

        // ----------------------------------------------------------------------------------
        // --- We only want to normalize on the number of nodes included in the report.
        // --- If GetNumNodesIncluded() == 0, then we are doing all nodes in the simulation.
        // --- Have to put this here because CheckForValidNodeIDs() gets called after Initialize().
        // ----------------------------------------------------------------------------------
        uint32_t tmp_num_nodes = m_ReportFilter.GetNumNodesIncluded();
        if( tmp_num_nodes > 0 )
        {
            _nrmSize = tmp_num_nodes;
        }
        release_assert( _nrmSize );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Calling this here because we now know and have validated
        // !!! the number of nodes used in the report
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
                                                                std::vector<INodeEventContext*>& rNodeEventContextList,
                                                                ISimulationEventContext* pSimEventContext )
    {
        m_IsValidDay = m_ReportFilter.IsValidTime( pSimEventContext->GetSimulationTime() );
        m_Dt = dt;
    }

    void SpatialReportMalariaFiltered::BeginTimestep()
    {
        m_HasValidNode = false;
        if( m_IsValidDay && (m_IntervalTimer <= 0.0) )
        {
            m_IntervalTimer += m_ReportingInterval;
            m_TimeElapsed = 0.0;
        }
        SpatialReportMalaria::BeginTimestep();
    }

    void SpatialReportMalariaFiltered::LogNodeData( INodeContext* pNC )
    {
        if( m_IsValidDay && m_ReportFilter.IsValidNode( pNC->GetEventContext() ) )
        {
            m_HasValidNode = true;
            SpatialReportMalaria::LogNodeData( pNC );
        }
    }

    void SpatialReportMalariaFiltered::LogIndividualData( IIndividualHuman* individual )
    {
        if( !m_IsValidDay ) return;
        if( !m_ReportFilter.IsValidNode( individual->GetEventContext()->GetNodeEventContext() ) ) return;
        if( !m_ReportFilter.IsValidHuman( individual ) ) return;

        SpatialReportMalaria::LogIndividualData( individual );
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

    void SpatialReportMalariaFiltered::WriteHeaderParameters( std::ofstream* file )
    {
        float start_day = m_ReportFilter.GetStartDay();
        SpatialReportMalaria::WriteHeaderParameters( file );
        file->write( (char*)&start_day, sizeof( float ) );
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

    std::string SpatialReportMalariaFiltered::GetReportName() const
    {
        return m_ReportFilter.GetNewReportName( SpatialReportMalaria::GetReportName() );
    }
}
