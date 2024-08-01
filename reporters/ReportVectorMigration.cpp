
#include "stdafx.h"

#include "ReportVectorMigration.h"

#include "report_params.rc"
#include "VectorCohortIndividual.h"
#include "IMigrate.h"
#include "ISimulationContext.h"
#include "IdmDateTime.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "SimulationConfig.h"
#include "IdmString.h"

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
#define DEFAULT_NAME ("ReportVectorMigration.csv")

    BEGIN_QUERY_INTERFACE_DERIVED( ReportVectorMigration, BaseTextReport )
        HANDLE_INTERFACE( IVectorMigrationReporting )
        HANDLE_INTERFACE( IReport )
        HANDLE_INTERFACE( IConfigurable )
    END_QUERY_INTERFACE_DERIVED( ReportVectorMigration, BaseTextReport )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( ReportVectorMigration )
#endif

    ReportVectorMigration::ReportVectorMigration()
        : BaseTextReport(DEFAULT_NAME)
        , m_StartDay( 0)
        , m_EndDay(FLT_MAX)
        , m_IsValidDay( false )
        , m_MustBeInState()
        , m_MustBeFromNode()
        , m_MustBeToNode()
        , m_IncludeGenomeData( false )
        , m_SpeciesList()
        , m_FilenameSuffix()
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
        initVectorConfig("Must_Be_In_State",
            m_MustBeInState,
            inputJson,
            MetadataDescriptor::VectorOfEnum("VectorStateEnum", RVM_Must_Be_In_State_DESC_TEXT, MDD_ENUM_ARGS(VectorStateEnum)));
        initConfigTypeMap("Must_Be_From_Node", &m_MustBeFromNode, RVM_Must_Be_From_Node_DESC_TEXT, 1, INT_MAX, false);
        initConfigTypeMap("Must_Be_To_Node", &m_MustBeToNode, RVM_Must_Be_To_Node_DESC_TEXT, 1, INT_MAX, false);
        initConfigTypeMap("Include_Genome_Data", &m_IncludeGenomeData, RVM_Include_Genome_Data_DESC_TEXT, false);
        initConfigTypeMap("Species_List", &m_SpeciesList, RVS_Species_List_DESC_TEXT);
        initConfigTypeMap("Filename_Suffix", &m_FilenameSuffix, ReportFilter_Filename_Suffix_DESC_TEXT, "");
        
        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_StartDay >= m_EndDay )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Start_Day", m_StartDay, "End_Day", m_EndDay );
            }
            for (auto state : m_MustBeInState)
            {
                switch (state)
                {
                    case VectorStateEnum::STATE_EGG:
                    {
                        throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__,
                            "'STATE_EGG' is not a valid state for ReportVectorMigration.\n");
                    }
                    case VectorStateEnum::STATE_LARVA:
                    {
                        throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__,
                            "'STATE_LARVA' is not a valid state for ReportVectorMigration.\n");
                    }
                    case VectorStateEnum::STATE_IMMATURE:
                    {
                        throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__,
                            "'STATE_IMMATURE' is not a valid state for ReportVectorMigration.\n");
                    }
                    case VectorStateEnum::STATE_MALE:
                    case VectorStateEnum::STATE_ADULT:
                    case VectorStateEnum::STATE_INFECTIOUS:
                    case VectorStateEnum::STATE_INFECTED:
                        break;
                }
            }
            if (!m_FilenameSuffix.empty())
            {
                IdmString tmp_report_name = DEFAULT_NAME;
                std::vector<IdmString> parts = tmp_report_name.split('.');
                release_assert(parts.size() == 2);
                std::string output_fn = parts[0];
                output_fn += "_" + m_FilenameSuffix + ".csv";
                SetReportName(output_fn);
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
        header << "Time" << ","
               << "ID" << ","
               << "FromNodeID" << ","
               << "ToNodeID" << ","
               << "State" << ",";

        if (m_IncludeGenomeData)
        {
            header << "Genome" << "," ;
        }

        header << "Species"        << ","
               << "Age"            << ","
               << "Population" ;

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

        VectorStateEnum::Enum state = pvc->GetState();
        if (!m_MustBeInState.empty())
        {
            if (!std::count(m_MustBeInState.begin(), m_MustBeInState.end(), state)) 
            {
                // state is not one of the required states
                return;
            }
        }

        uint32_t vci_id = pvc->GetID();
        int from_node_id = pSim->GetNodeExternalID( nodeSuid ) ;
        if (!m_MustBeFromNode.empty())
        {
            if (!std::count(m_MustBeFromNode.begin(), m_MustBeFromNode.end(), from_node_id))
            {
                // from_node_id is not one we want in the report
                return;
            }
        }
        int to_node_id = pSim->GetNodeExternalID( pim->GetMigrationDestination() ) ;
        if (!m_MustBeToNode.empty())
        {
            if (!(std::find(m_MustBeToNode.begin(), m_MustBeToNode.end(), to_node_id) != m_MustBeToNode.end()))
            {
                // to_node_id is not one we want in the report
                return;
            }
        }
        std::string species = pvc->GetSpecies();
        if (!m_SpeciesList.empty())
        {
            if (!(std::find(m_SpeciesList.begin(), m_SpeciesList.end(), species) != m_SpeciesList.end()))
            {
                // if not species we're interested in do not add to report
                return;
            }
        }
        float age = pvc->GetAge();
        int num = pvc->GetPopulation();

        GetOutputStream() << currentTime
            << "," << vci_id
            << "," << from_node_id
            << "," << to_node_id
            << "," << VectorStateEnum::pairs::get_keys()[state];
        if (m_IncludeGenomeData)
        {
            const VectorGeneCollection &gene_collection = GET_CONFIGURABLE(SimulationConfig)->vector_params->vector_species.GetSpecies(species).genes;
            VectorGenome genome = pvc->GetGenome();
            std::string gene_name = gene_collection.GetGenomeName(genome);
            GetOutputStream() << "," << gene_name;
        }
        GetOutputStream() << "," << species
                   << "," << age 
                   << "," << num 
                   << endl;
    }
}
