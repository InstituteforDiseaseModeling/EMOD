
#include "stdafx.h"

#include "ReportVectorMigration.h"

#include "report_params.rc"
#include "VectorCohortIndividual.h"
#include "IMigrate.h"
#include "ISimulationContext.h"
#include "IdmDateTime.h"

#ifdef _REPORT_DLL
#include "DllInterfaceHelper.h"
#include "FactorySupport.h" // for DTK_DLLEXPORT
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportVectorMigration" ) // <<< Name of this file

namespace Kernel
{
#ifdef _REPORT_DLL

// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "VECTOR_SIM", "MALARIA_SIM", nullptr };// <<< Types of simulation the report is to be used with

instantiator_function_t rif = []()
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

DTK_DLLEXPORT char*
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
}

DTK_DLLEXPORT void
GetSupportedSimTypes(char* simTypes[])
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
// --- ReportVectorMigration Methods
// ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportVectorMigration, BaseTextReport )
        HANDLE_INTERFACE( IVectorMigrationReporting )
        HANDLE_INTERFACE( IReport )
        HANDLE_INTERFACE( IConfigurable )
    END_QUERY_INTERFACE_DERIVED( ReportVectorMigration, BaseTextReport )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( ReportVectorMigration )
#endif

    ReportVectorMigration::ReportVectorMigration()
        : BaseTextReport( "ReportVectorMigration.csv" )
        , m_StartDay( 365.0 )
        , m_EndDay( 372 )
        , m_IsValidDay( false )
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );

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

    bool ReportVectorMigration::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Start_Day", &m_StartDay, Report_Start_Day_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "End_Day",   &m_EndDay,   Report_End_Day_DESC_TEXT,   0.0f, FLT_MAX, FLT_MAX );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_StartDay >= m_EndDay )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Start_Day", m_StartDay, "End_Day", m_EndDay );
            }
        }
        return ret;
    }

    void ReportVectorMigration::UpdateEventRegistration( float currentTime,
                                                         float dt,
                                                         std::vector<INodeEventContext*>& rNodeEventContextList,
                                                         ISimulationEventContext* pSimEventContext )
    {
        m_IsValidDay = (m_StartDay <= currentTime) && (currentTime <= m_EndDay);
    }

    std::string ReportVectorMigration::GetHeader() const
    {
        std::stringstream header ;
        header << "Time"           << ","
               << "ID"             << ","
               << "FromNodeID"     << ","
               << "ToNodeID"       << ","
               << "MigrationType"  << ","
               << "Species"        << ","
               << "Age"            << ","
               << "Population"
               ;

        return header.str();
    }

    void ReportVectorMigration::LogVectorMigration( ISimulationContext* pSim, float currentTime, const suids::suid& nodeSuid, IVectorCohort* pvc )
    {
        if( !m_IsValidDay )
        {
            return;
        }

        IMigrate * pim = NULL;
        if (s_OK != pvc->QueryInterface(GET_IID(IMigrate), (void**)&pim) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pvc", "IMigrate", "IVectorCohort");
        }

        uint32_t vci_id = pvc->GetID();
        int from_node_id = pSim->GetNodeExternalID( nodeSuid ) ;
        int to_node_id = pSim->GetNodeExternalID( pim->GetMigrationDestination() ) ;
        int mig_type = pim->GetMigrationType() ;
        VectorStateEnum::Enum state = pvc->GetState();
        std::string species = pvc->GetSpecies();
        float age = pvc->GetAge();
        int num = pvc->GetPopulation();

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
                   << "," << num 
                   << endl;
    }
}
