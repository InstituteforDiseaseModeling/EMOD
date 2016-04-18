/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Simulation.h"

#include <iomanip>      // std::setprecision

#include "FileSystem.h"
#include "Debug.h"
#include "Report.h"
#include "BinnedReport.h"
#include "DemographicsReport.h"
#include "SpatialReport.h"
#include "PropertyReport.h"
#include "Exceptions.h"
#include "Instrumentation.h"
#include "InterventionEnums.h"
#include "IMigrationInfo.h"
#include "Node.h"
#include "NodeDemographics.h"
#include "RANDOM.h"
#include "SimulationConfig.h"
#include "SimulationEventContext.h"
#include "NodeInfo.h"
#include "ReportEventRecorder.h"
#include "Individual.h"
#include "LoadBalanceScheme.h"

#include "DllLoader.h"

#include "BinaryArchiveWriter.h"
#include "BinaryArchiveReader.h"
#include "JsonRawWriter.h"
#include "JsonRawReader.h"
#include "MpiDataExchanger.h"
#include "IdmMpi.h"

#include <chrono>
typedef std::chrono::high_resolution_clock _clock;

using namespace std;

static const char * _module = "Simulation";

namespace Kernel
{
    // Enable querying of interfaces from Simulation objects
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Simulation,Simulation)
    BEGIN_QUERY_INTERFACE_BODY(Simulation)
        HANDLE_INTERFACE(IGlobalContext)
        HANDLE_INTERFACE(ISimulation)
        HANDLE_INTERFACE(ISimulationContext)
        HANDLE_ISUPPORTS_VIA(ISimulationContext)
    END_QUERY_INTERFACE_BODY(Simulation)

    //------------------------------------------------------------------
    //   Initialization methods
    //------------------------------------------------------------------

    Simulation::Simulation()
        : serializationMask(SerializationFlags(uint32_t(SerializationFlags::Population) | uint32_t(SerializationFlags::Parameters)))
        , nodes()
        , nodeRankMap()
        , node_events_added()
        , node_events_to_be_processed()
        , node_event_context_list()
        , nodeid_suid_map()
        , migratingIndividualQueues()
        , m_simConfigObj(nullptr)
        , m_interventionFactoryObj(nullptr)
        , demographicsContext(nullptr)
        , infectionSuidGenerator(EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks)
        , individualHumanSuidGenerator(EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks)
        , nodeSuidGenerator(EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks)
        , campaignFilename()
        , loadBalanceFilename()
        , rng(nullptr)
        , reports()
        , individual_data_reports()
        , reportClassCreator(nullptr)
        , binnedReportClassCreator(nullptr)
        , spatialReportClassCreator(nullptr)
        , propertiesReportClassCreator(nullptr)
        , demographicsReportClassCreator(nullptr)
        , eventReportClassCreator(nullptr)
        , event_coordinators()
        , campaign_events()
        , event_context_host(nullptr)
        , Ind_Sample_Rate(1.0f)
        , currentTime()
        , random_type(RandomType::USE_PSEUDO_DES)
        , sim_type(SimType::GENERIC_SIM)
        , demographic_tracking(false)
        , enable_spatial_output(false)
        , enable_property_output(false)
        , enable_default_report(false)
        , enable_event_report(false)
        , campaign_filename()
        , loadbalance_filename()
        , Run_Number(0)
        , can_support_family_trips( false )
        , demographics_factory(nullptr)
        , new_node_observers()
    {
        LOG_DEBUG( "CTOR\n" );

        reportClassCreator              = Report::CreateReport;
        binnedReportClassCreator        = BinnedReport::CreateReport;
        spatialReportClassCreator       = SpatialReport::CreateReport;
        propertiesReportClassCreator    = PropertyReport::CreateReport;
        demographicsReportClassCreator  = DemographicsReport::CreateReport;
        eventReportClassCreator         = ReportEventRecorder::CreateReport;

        nodeRankMap.SetNodeInfoFactory( this );

        initConfigTypeMap( "Enable_Default_Reporting", &enable_default_report, Enable_Default_Reporting_DESC_TEXT, true );
        initConfigTypeMap( "Enable_Demographics_Reporting", &demographic_tracking, Enable_Demographics_Reporting_DESC_TEXT, true );
        initConfigTypeMap( "Report_Event_Recorder", &enable_event_report, Report_Event_Recorder_DESC_TEXT,   false);
        initConfigTypeMap( "Enable_Spatial_Output", &enable_spatial_output, Enable_Spatial_Output_DESC_TEXT, false );
        initConfigTypeMap( "Enable_Property_Output", &enable_property_output, Enable_Property_Output_DESC_TEXT, false );
        initConfigTypeMap( "Campaign_Filename", &campaign_filename, Campaign_Filename_DESC_TEXT );
        initConfigTypeMap( "Load_Balance_Filename", &loadbalance_filename, Load_Balance_Filename_DESC_TEXT );
        initConfigTypeMap( "Base_Individual_Sample_Rate", &Ind_Sample_Rate, Base_Individual_Sample_Rate_DESC_TEXT, 0.0f, 1.0f, 1.0f ); 
        initConfigTypeMap( "Run_Number", &Run_Number, Run_Number_DESC_TEXT, 0, INT_MAX, 1 );
    }

    Simulation::~Simulation()
    {
        LOG_DEBUG( "DTOR\n" );
        for (auto& entry : nodes)
        {
            delete entry.second;
        }
        nodes.clear();

        delete demographics_factory;
        demographics_factory = nullptr;

        if (rng) delete rng;

        delete event_context_host;
        event_context_host = nullptr;

        for (auto report : reports)
        {
            LOG_DEBUG_F( "About to delete report = %s\n", report->GetReportName().c_str() );
            delete report;
        }
        reports.clear();
    }

    bool
    Simulation::Configure(
        const Configuration * inputJson
    )
    {
        initConfig( "Simulation_Type", sim_type, inputJson, MetadataDescriptor::Enum("sim_type", Simulation_Type_DESC_TEXT, MDD_ENUM_ARGS(SimType)) ); // simulation only (???move)
        bool ret = JsonConfigurable::Configure( inputJson );
        return ret;
    }

    Simulation *Simulation::CreateSimulation()
    {
        Simulation *newsimulation = _new_ Simulation();
        newsimulation->Initialize();

        return newsimulation;
    }

    Simulation *Simulation::CreateSimulation(const ::Configuration *config)
    {
        Simulation *newsimulation = _new_ Simulation();
        if (newsimulation)
        {
            // This sequence is important: first
            // Creation-->Initialization-->Validation
            newsimulation->Initialize(config);
            if(!ValidateConfiguration(config))
            {
                delete newsimulation;
                newsimulation = nullptr;
            }
        }

        return newsimulation;
    }

    bool Simulation::ValidateConfiguration(const ::Configuration *config)
    {
        // NOTE: ClimateFactory::climate_structure, configured from the parameter "Climate_Model",
        //       is only available after the factory is created in Simulation::populateFromDemographics
        //       SimulationConfig and the various static individual/infection/susceptibility parameters
        //       should be initialized by this point (see Simulation::Initialize).
        //       However, this function still has access to the complete set of configurable parameters
        //       from the  json::Configuration argument that is passed in by the SimulationFactory.

        return true;
    }

    void Simulation::Initialize()
    {
        LOG_DEBUG( "Initialize\n" );
    }

    void Simulation::Initialize(const ::Configuration *config)
    {
        Configure( config );
        // Let's try generalizing this somehow!!!
        IndividualHumanConfig fakeHuman;
        LOG_INFO( "Calling Configure on fakeHuman\n" );
        fakeHuman.Configure( config );

        Kernel::SimulationConfig* SimConfig = Kernel::SimulationConfigFactory::CreateInstance(EnvPtr->Config);
        if (SimConfig)
        {
            Environment::setSimulationConfig(SimConfig);
            m_simConfigObj = SimConfig;
        }
        else
        {
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "Failed to create SimulationConfig instance" );
        }
        LOG_DEBUG( "Initialize with config\n" );

        m_interventionFactoryObj = InterventionFactory::getInstance();

        setupMigrationQueues();
        setupEventContextHost();
        setupRng(); 
        setParams(config); 
        initSimulationState();
        Reports_CreateBuiltIn();
        Reports_ConfigureBuiltIn();
        Reports_CreateCustom();
        Reports_FindReportsCollectingIndividualData( 0.0, 0.0 );
    }

    INodeInfo* Simulation::CreateNodeInfo()
    {
        INodeInfo* pin = new NodeInfo();
        return pin ;
    }

    INodeInfo* Simulation::CreateNodeInfo( int rank, INodeContext* pNC )
    {
        INodeInfo* pin = new NodeInfo( rank, pNC );
        return pin ;
    }

    void Simulation::setupMigrationQueues()
    {
        migratingIndividualQueues.resize(EnvPtr->MPI.NumTasks); // elements are instances not pointers
    }

    void Simulation::setupEventContextHost()
    {
        event_context_host = _new_ SimulationEventContextHost(this);
    }

    void Simulation::setupRng()
    {

        if (random_type == RandomType::USE_PSEUDO_DES)
        { 
            uint16_t randomseed[2];
            randomseed[0] = uint16_t(Run_Number);
            randomseed[1] = uint16_t(EnvPtr->MPI.Rank);
            rng = _new_ PSEUDO_DES(*reinterpret_cast<uint32_t*>(randomseed));
            const_cast<Environment*>(Environment::getInstance())->RNG = rng;
            

            LOG_INFO("Using PSEUDO_DES random number generator.\n");
        }
        else
        {
            std::ostringstream oss;
            oss << "Error in " << __FUNCTION__ << ".  Only USE_PSEUDO_DES is currently supported for key 'Random_Type'." << std::endl;
            throw NotYetImplementedException(  __FILE__, __LINE__, __FUNCTION__, oss.str().c_str() );
        }
    }

    void Simulation::setParams(const ::Configuration *config)
    {
        if( campaign_filename.empty() )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Campaign_Filename' is empty.  You must have a file." );
        }

        loadBalanceFilename         = FileSystem::Concat( EnvPtr->InputPath, loadbalance_filename );
        campaignFilename            = campaign_filename ;

        currentTime.time      =  m_simConfigObj->starttime;
    }

    void Simulation::initSimulationState()
    {
        // right now this function absorbs whatever was left from the original constructor semantics
        // the intent is that this sets any member variables not set up already that are part of the 
        // initial simulation state

        currentTime.timestep = 0;
    }

    //------------------------------------------------------------------
    //   Reporting methods
    //------------------------------------------------------------------

    void Simulation::Reports_CreateBuiltIn()
    {
        // Check reporter_plugins directory and use report classes from there if any exists

        LOG_DEBUG( "Reports_CreateBuiltIn()\n" );

        if( enable_default_report )
        {
            // Default report
            IReport * report = (*reportClassCreator)();
            release_assert(report);
            reports.push_back(report);
        }

        if(enable_property_output)
        {
            IReport * prop_report = (*propertiesReportClassCreator)();
            release_assert(prop_report);
            reports.push_back(prop_report);
        }

        if(enable_spatial_output)
        {
            IReport * spatial_report = (*spatialReportClassCreator)();
            release_assert(spatial_report);
            reports.push_back(spatial_report);
        }

        if( enable_event_report )
        {
            IReport * event_report = (*eventReportClassCreator)();
            release_assert(event_report);
            reports.push_back(event_report);
        }

        if(demographic_tracking)
        {
            IReport * binned_report = (*binnedReportClassCreator)();
            release_assert(binned_report);
            reports.push_back(binned_report);

            IReport* demo_report = (*demographicsReportClassCreator)();
            release_assert(demo_report);
            reports.push_back(demo_report);
        }
    }

    void Simulation::RegisterNewNodeObserver(void* id, Kernel::ISimulation::callback_t observer)
    {
        new_node_observers[id] = observer;
    }

    void Simulation::UnregisterNewNodeObserver(void* id)
    {
        if (new_node_observers.erase(id) == 0)
        {
            LOG_WARN_F("%s: Didn't find entry for id %08X in observer map.", __FUNCTION__, id);
        }
    }

    void Simulation::DistributeEventToOtherNodes( const std::string& rEventName, INodeQualifier* pQualifier )
    {
        release_assert( pQualifier );

        for( auto entry : nodeRankMap.GetRankMap() )
        {
            INodeInfo* pni = entry.second ;
            if( pQualifier->Qualifies( *pni ) )
            {
                // -------------------------------------------------------------------------------------
                // --- One could use a map to keep a node from getting more than one event per timestep
                // --- but I think the logic for that belongs in the object processing the event
                // -------------------------------------------------------------------------------------
                node_events_added[ nodeRankMap.GetRankFromNodeSuid( pni->GetSuid() ) ].Add( pni->GetSuid(), rEventName ) ;
            }
        }
    }


    void Simulation::UpdateNodeEvents()
    {
        WithSelfFunc to_self_func = [this](int myRank) 
        { 
            //do nothing
        }; 

        // -------------------------------------------------------------
        // --- Send Event Triggers destined for nodes not on this processor
        // -------------------------------------------------------------
        SendToOthersFunc to_others_func = [this](IArchive* writer, int toRank)
        {
            *writer & node_events_added[toRank];
        };

        // -----------------------------------------------------------------
        // --- Receive the Event Triggers destined for the nodes on this processor
        // -----------------------------------------------------------------
        ReceiveFromOthersFunc from_others_func = [this](IArchive* reader, int fromRank)
        {
            EventsForOtherNodes efon ;
            *reader & efon;
            node_events_added[ EnvPtr->MPI.Rank ].Update( efon );
        };

        ClearDataFunc clear_data_func = [this](int rank)
        {
        };

        MpiDataExchanger exchanger( "EventsForOtherNodes", to_self_func, to_others_func, from_others_func, clear_data_func );
        exchanger.ExchangeData( this->currentTime );

        // ---------------------------------------------------
        // --- Update the events to be processed during the 
        // --- next time step for the nodes on this processor
        // ---------------------------------------------------
        node_events_to_be_processed.clear();
        for( auto entry : node_events_added[ EnvPtr->MPI.Rank ].GetMap() )
        {
            suids::suid node_id = entry.first;
            auto& event_name_list = entry.second;
            for( auto event_name : event_name_list )
            {
                node_events_to_be_processed[ node_id ].push_back( event_name );
            }
        }
        node_events_added.clear();

    }

    void Simulation::Reports_ConfigureBuiltIn()
    {
        for( auto report : reports )
        {
            report->Configure( EnvPtr->Config );
        }
    }

    void Simulation::Reports_CreateCustom()
    {
        // -------------------------------------------------------------
        // --- Allow the user to indicate that they do not want to use
        // --- any custom reports even if DLL's are present.
        // -------------------------------------------------------------
        if( EnvPtr->Config->Exist( "Custom_Reports_Filename" ) )
        {
            std::string custom_reports_filename = GET_CONFIG_STRING( EnvPtr->Config, "Custom_Reports_Filename" );
            if( custom_reports_filename == "NoCustomReports" )
            {
                return ;
            }
        }

        ReportInstantiatorMap report_instantiator_map ;
        SimType::Enum st_enum = m_simConfigObj->sim_type;
#ifdef WIN32
        DllLoader dllLoader(SimType::pairs::lookup_key(st_enum));
        if( !dllLoader.LoadReportDlls( report_instantiator_map ) )
        {
            LOG_WARN_F("Failed to load reporter emodules for SimType: %s from path: %s\n" , SimType::pairs::lookup_key(st_enum), dllLoader.GetEModulePath(REPORTER_EMODULES).c_str());
        }
#endif
        Reports_Instantiate( report_instantiator_map );
    }

    void Simulation::Reports_FindReportsCollectingIndividualData( float currentTime, float dt )
    {
        // ---------------------------------------------------------------------
        // --- Get the subset of reports that are collecting individual data
        // --- This allows us to avoid calling LogIndividualData() for reports
        // --- that just have no-ops on every individual.
        // ---------------------------------------------------------------------
        individual_data_reports.clear();
        for( auto report : reports )
        {
            if( report->IsCollectingIndividualData( currentTime, dt ) )
            {
                individual_data_reports.push_back( report );
            }
        }
    }

    Configuration* Simulation::Reports_GetCustomReportConfiguration()
    {
        Configuration* p_cr_config = nullptr ;

        if( EnvPtr->Config->Exist( "Custom_Reports_Filename" ) )
        {
            std::string custom_reports_filename = GET_CONFIG_STRING( EnvPtr->Config, "Custom_Reports_Filename" );
            LOG_INFO_F("Looking for custom reports file = %s\n", custom_reports_filename.c_str());
            if( FileSystem::FileExists( custom_reports_filename ) )
            {
                LOG_INFO_F("Found custom reports file = %s\n", custom_reports_filename.c_str());
                // it is extremely unlikely that this will return null.  It will throw an exception if an error occurs.
                Configuration* p_config = Configuration::Load( custom_reports_filename );
                if( !p_config ) 
                {
                    throw Kernel::InitializationException( __FILE__, __LINE__, __FUNCTION__, custom_reports_filename.c_str() );
                }
                p_cr_config = Configuration::CopyFromElement((*p_config)["Custom_Reports"]);
                delete p_config ;
            }
        }
        return p_cr_config ;
    }

    void Simulation::Reports_Instantiate( ReportInstantiatorMap& rReportInstantiatorMap )
    {
        Configuration* p_cr_config = Reports_GetCustomReportConfiguration();

        bool load_all_reports = (p_cr_config == nullptr) ||
                                !p_cr_config->Exist( "Use_Explicit_Dlls" ) ||
                                (int(p_cr_config->operator[]( "Use_Explicit_Dlls" ).As<json::Number>()) != 1) ;

        LOG_INFO_F("Found %d Custom Report DLL's to consider loading, load_all_reports=%d\n", rReportInstantiatorMap.size(), load_all_reports );
        for( auto ri_entry : rReportInstantiatorMap )
        {
            std::string class_name = ri_entry.first ;
            try
            {
                if( (p_cr_config != nullptr) && p_cr_config->Exist( class_name ) )
                {
                    LOG_INFO_F("Found custom report data for %s\n", class_name.c_str());
                    json::QuickInterpreter dll_data = p_cr_config->operator[]( class_name ).As<json::Object>() ;
                    if( int(dll_data["Enabled"].As<json::Number>()) != 0 )
                    {
                        json::Array report_data = dll_data["Reports"].As<json::Array>() ;
                        for( int i = 0 ; i < report_data.Size() ; i++ )
                        {
                            Configuration* p_cfg = Configuration::CopyFromElement( report_data[i] );

                            IReport* p_cr = ri_entry.second(); // creates report object
                            p_cr->Configure( p_cfg );
                            reports.push_back( p_cr );
                            delete p_cfg ;
                            p_cfg = nullptr;
                        }
                    }
                }
                else if( load_all_reports )
                {
                    LOG_WARN_F("Did not find report configuration for report DLL %s.  Creating report with defaults.\n", class_name.c_str());

                    json::Object empty_json_obj ;
                    Configuration* p_cfg = Configuration::CopyFromElement( empty_json_obj );

                    IReport* p_cr = ri_entry.second();  // creates report object
                    p_cr->Configure( p_cfg );
                    reports.push_back( p_cr );
                    delete p_cfg ;
                    p_cfg = nullptr;
                }
            }
            catch( json::Exception& e )
            {
                std::stringstream ss ;
                ss << "Error occured reading report data for " << class_name << ".  Error: " << e.what() << std::endl ;
                throw InitializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        delete p_cr_config;
        p_cr_config = nullptr;
    }

    void Simulation::Reports_UpdateEventRegistration( float _currentTime, float dt )
    {
        for (auto report : reports)
        {
            report->UpdateEventRegistration( _currentTime, dt, node_event_context_list );
        }
    }

    void Simulation::Reports_BeginTimestep()
    {
        for (auto report : reports)
        {
            release_assert(report);
            report->BeginTimestep();
        }
    }

    void Simulation::Reports_EndTimestep( float _currentTime, float dt )
    {
        for (auto report : reports)
        {
            release_assert(report);
            report->EndTimestep( _currentTime, dt );
        }
    }

    void Simulation::Reports_LogNodeData( INodeContext* n )
    {
        for (auto report : reports)
        {
            report->LogNodeData( n );
        }
    }

    void Simulation::PrintTimeAndPopulation()
    {
        // print out infections and population out
        int stat_pop = 0, infected = 0;
        for (auto& entry : nodes)
        {
            INodeContext* n = entry.second;
            stat_pop += n->GetStatPop();
            infected += n->GetInfected();
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << "Update(): Time: " << float(currentTime.time);
        if( currentTime.getBaseYear() > 0 )
        {
            oss << std::fixed << " Year: " << currentTime.Year();
        }
        oss << std::fixed << " Rank: " << EnvPtr->MPI.Rank << " StatPop: " << stat_pop << " Infected: " << infected << std::endl;
        LOG_INFO( oss.str().c_str() );
    }

    void Simulation::WriteReportsData()
    {
        //std::cout << "There are " << reports.size() << " reports to finalize." << std::endl;
        for (auto report : reports)
        {
            report->Reduce();

            // the rest only make sense on rank 0
            if (EnvPtr->MPI.Rank == 0)
            {
                LOG_INFO_F( "Finalizing '%s' reporter.\n", report->GetReportName().c_str() );
                report->Finalize();
            }
        }
    }

    //------------------------------------------------------------------
    //   Every timestep Update() method
    //------------------------------------------------------------------

// Use with __try {} __except(filter(GetExceptionCode(), GetExceptionInformation())) { MPI_Abort(EnvPtr->world, -1); }
//
//    static char* section;
//
//    static int filter(unsigned int code, struct _EXCEPTION_POINTERS* ep)
//    {
//        cout << '[' << EnvPtr->MPI.Rank << "] " << "Section: " << section << endl; cout.flush();
//        cout << '[' << EnvPtr->MPI.Rank << "] " << "Exception code      = " << code << endl; cout.flush();
//        cout << '[' << EnvPtr->MPI.Rank << "] " << "EP.ExceptionCode    = " << ep->ExceptionRecord->ExceptionCode << endl; cout.flush();
//        cout << '[' << EnvPtr->MPI.Rank << "] " << "EP.ExceptionAddress = " << ep->ExceptionRecord->ExceptionAddress << endl; cout.flush();
//
//        void* callers[32];
//        auto frames = CaptureStackBackTrace( 0, 32, callers, nullptr);
//        for (size_t i = 0; i < frames; i++) {
//            cout << '[' << i << "] " << callers[i] << endl;
//        }
//        cout.flush();
//
//        return EXCEPTION_EXECUTE_HANDLER;
//    }

    void Simulation::Update(float dt)
    {
        Reports_UpdateEventRegistration( currentTime.time, dt );
        Reports_FindReportsCollectingIndividualData( currentTime.time, dt );

        // -----------------
        // --- Update Events
        // -----------------
        release_assert(event_context_host);
        event_context_host->Update(dt);

        Reports_BeginTimestep();

        // -----------------
        // --- Update Nodes
        // -----------------
        for (auto iterator = nodes.rbegin(); iterator != nodes.rend(); ++iterator)
        {
            INodeContext* n = iterator->second;
            release_assert(n);
            n->AddEventsFromOtherNodes( node_events_to_be_processed[ n->GetSuid() ] );
            n->Update(dt);

            Reports_LogNodeData( n );
        }

        // -----------------------
        // --- Resolve Migration
        // -----------------------
        REPORT_TIME( ENABLE_DEBUG_MPI_TIMING, "resolveMigration", resolveMigration() );

        for (auto iterator = nodes.rbegin(); iterator != nodes.rend(); ++iterator)
        {
            INodeContext *n = iterator->second;
            nodeRankMap.Update( n );
        }
        nodeRankMap.Sync( currentTime );

        UpdateNodeEvents();

        // -------------------
        // --- Increment Time
        // -------------------
        currentTime.Update( dt );

        // ----------------------------------------------------------
        // --- Output Information for the end of the update/timestep
        // ----------------------------------------------------------
        PrintTimeAndPopulation();

        Reports_EndTimestep( currentTime.time, dt );

        if(EnvPtr->Log->CheckLogLevel(Logger::DEBUG, "Memory"))
        {
            MemoryGauge::PrintMemoryUsage();
            MemoryGauge::PrintMemoryFree();
        }

        // Unconditionally checking against potential memory blowup with minimum cost
        MemoryGauge::CheckMemoryFailure();
    }

    //------------------------------------------------------------------
    //   First timestep Populate() methods
    //------------------------------------------------------------------

    bool Simulation::Populate()
    {
        LOG_DEBUG("Calling populateFromDemographics()\n");

        // Populate nodes
        LOG_INFO_F("Campaign file name identified as: %s\n", campaignFilename.c_str());
        int nodes = populateFromDemographics(campaignFilename.c_str(), loadBalanceFilename.c_str());
        LOG_INFO_F("populateFromDemographics() generated %d nodes.\n", nodes);

        LOG_INFO_F("Rank %d contributes %d nodes...\n", EnvPtr->MPI.Rank, nodeRankMap.Size());
        EnvPtr->Log->Flush();
        LOG_INFO_F("Merging node rank maps...\n");
        nodeRankMap.MergeMaps(); // merge rank maps across all processors
        LOG_INFO_F("Merged rank %d map now has %d nodes.\n", EnvPtr->MPI.Rank, nodeRankMap.Size());

        if (nodeRankMap.Size() < 500)
            LOG_INFO_F("Rank %d map contents:\n%s\n", EnvPtr->MPI.Rank, nodeRankMap.ToString().c_str());
        else 
            LOG_INFO("(Rank map contents not displayed due to large (> 500) number of entries.)\n");

        // We'd like to be able to run even if a processor has no nodes, but there are other issues.
        // So for now just bail...
        if(nodes <= 0)
        {
            LOG_WARN_F("Rank %d wasn't assigned any nodes! (# of procs is too big for simulation?)\n", EnvPtr->MPI.Rank);
            return false;
        }

        for (auto report : reports)
        {
            LOG_DEBUG( "Initializing report...\n" );
            report->Initialize( nodeRankMap.Size() );
            LOG_INFO_F( "Initialized '%s' reporter\n", report->GetReportName().c_str() );
        }

        return true;
    }

    void Simulation::MergeNodeIdSuidBimaps(nodeid_suid_map_t& local_map, nodeid_suid_map_t& merged_map)
    {
        merged_map = local_map;

        if (EnvPtr->MPI.NumTasks > 1)
        {
            auto json_writer = new JsonRawWriter();
            IArchive& writer_archive = *static_cast<IArchive*>(json_writer);
            size_t count = local_map.size();
            writer_archive.startArray( count );
            LOG_VALID_F( "Serializing %d id-suid bimap entries.\n", count );
            for (auto& entry : local_map)
            {
#if defined(WIN32)
                writer_archive.startObject();
                    writer_archive.labelElement( "id" ) & uint32_t(entry.left);
                    uint32_t suid = entry.right.data;
                    writer_archive.labelElement( "suid") & suid;
                writer_archive.endObject();
#endif
            }
            writer_archive.endArray();

            for (int rank = 0; rank < EnvPtr->MPI.NumTasks; ++rank)
            {
                if (rank == EnvPtr->MPI.Rank)
                {
                    const char* buffer = writer_archive.GetBuffer();
                    uint32_t byte_count = writer_archive.GetBufferSize();
                    LOG_VALID_F( "Broadcasting serialized bimap (%d bytes)\n", byte_count );
                    EnvPtr->MPI.p_idm_mpi->PostChars( const_cast<char*>(buffer), byte_count, rank );
                }
                else
                {
                    std::vector<char> received;
                    EnvPtr->MPI.p_idm_mpi->GetChars( received, rank );
                    auto json_reader = new JsonRawReader( received.data() );
                    IArchive& reader_archive = *static_cast<IArchive*>(json_reader);
                    size_t entry_count;
                    reader_archive.startArray( entry_count );
                        LOG_VALID_F( "Merging %d id-suid bimap entries from rank %d\n", entry_count, rank );
                        for (size_t i = 0; i < entry_count; ++i)
                        {
                            uint32_t id;
                            suids::suid suid;
                            reader_archive.startObject();
                                reader_archive.labelElement( "id" ) & id;
                                reader_archive.labelElement( "suid_data" ) & suid.data;
                            reader_archive.endObject();
                            merged_map.insert(nodeid_suid_pair(id, suid));
                        }
                    reader_archive.endArray();
                    delete json_reader;
                    //delete [] buffer;
                }
            }

            delete json_writer;
        }
    }

    IMigrationInfoFactory* Simulation::CreateMigrationInfoFactory ( const std::string& idreference,
                                                                    MigrationStructure::Enum ms,
                                                                    int torusSize )
    {
        IMigrationInfoFactory* pmf = MigrationFactory::ConstructMigrationInfoFactory( EnvPtr->Config, 
                                                                                      idreference,
                                                                                      m_simConfigObj->sim_type,
                                                                                      ms,
                                                                                      !(m_simConfigObj->demographics_initial),
                                                                                      torusSize );
        return pmf ;
    }

    void Simulation::LoadInterventions( const char* campaignfilename )
    {
        JsonConfigurable::_track_missing = false;
        // Set up campaign interventions from file
        release_assert( event_context_host );
        event_context_host->campaign_filename = campaignfilename;
        release_assert( m_simConfigObj );
        if (m_simConfigObj->interventions)
        {
            LOG_INFO_F( "Looking for campaign file %s\n", campaignfilename );

            if ( !FileSystem::FileExists( campaignfilename ) )
            {
                throw FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, campaignfilename );
            }
            else 
            {
                LOG_INFO("Found campaign file successfully.\n");
            }

            SimType::Enum st_enum = m_simConfigObj->sim_type;
#ifdef WIN32
            DllLoader dllLoader;
            if (!dllLoader.LoadInterventionDlls())
            {
                LOG_WARN_F("Failed to load intervention emodules for SimType: %s from path: %s\n", SimType::pairs::lookup_key(st_enum), dllLoader.GetEModulePath(INTERVENTION_EMODULES).c_str());
            }
#endif

            loadCampaignFromFile(campaignfilename);

            std::string transitions_file_path = FileSystem::Concat( Environment::getInstance()->OutputPath, std::string(Node::transitions_dot_json_filename) );
            if( FileSystem::FileExists( transitions_file_path ) )
            {
                loadCampaignFromFile(transitions_file_path);
            }
        }

        JsonConfigurable::_track_missing = true;
    }

    int Simulation::populateFromDemographics(const char* campaignfilename, const char* loadbalancefilename)
    {
        // Initialize node demographics from file
        demographics_factory = NodeDemographicsFactory::CreateNodeDemographicsFactory( &nodeid_suid_map, 
                                                                                       EnvPtr->Config,
                                                                                       m_simConfigObj->demographics_initial,
                                                                                       m_simConfigObj->default_torus_size,
                                                                                       m_simConfigObj->default_node_population );
        if (demographics_factory == nullptr)
        {
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "Failed to create NodeDemographicsFactory" );
        }

        demographicsContext = demographics_factory->CreateDemographicsContext();
        string idreference  = demographics_factory->GetIdReference();
        vector<uint32_t> nodeIDs = demographics_factory->GetNodeIDs();
        ClimateFactory * climate_factory = nullptr;
#ifndef DISABLE_CLIMATE
        // Initialize climate from file
        climate_factory = ClimateFactory::CreateClimateFactory(&nodeid_suid_map, EnvPtr->Config, idreference);
        if (climate_factory == nullptr)
        {
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "Failed to create ClimateFactory" );
        }
#endif
        // We can validate climate structure against sim_type now.

        // Initialize load-balancing scheme from file
        IInitialLoadBalanceScheme* p_lbs = LoadBalanceSchemeFactory::Create( loadbalancefilename, nodeIDs.size(), EnvPtr->MPI.NumTasks );
        nodeRankMap.SetInitialLoadBalanceScheme( p_lbs );

        // Delete any existing transitions.json file
        // TODO: only remove the transitions.json file if running on a single computer and single node 
        // If running on multiple computers/spatial multinode sims, there will be a problem where one computer deletes the transitions.json file when the other computers still need it

        // Anyone could delete the file, but we’ll delegate to rank 0
        if (EnvPtr->MPI.Rank == 0)
        {
            std::string transitions_file_path = FileSystem::Concat( Environment::getInstance()->OutputPath, std::string( Node::transitions_dot_json_filename ) );
            LOG_DEBUG_F( "Deleting any existing %s file.\n", transitions_file_path.c_str() );
            FileSystem::RemoveFile( transitions_file_path );
        }
        EnvPtr->MPI.p_idm_mpi->Barrier();

        // Add nodes according to demographics-and climate file specifications
        for (auto node_id : nodeIDs)
        {
            if (getInitialRankFromNodeId(node_id) == EnvPtr->MPI.Rank) // inclusion criteria to be added to this processor's shared memory space
            {
                suids::suid node_suid = GetNextNodeSuid();
                LOG_DEBUG_F( "Creating/adding new node: node_id = %d, node_suid = %lu\n", node_id, node_suid.data );
                nodeid_suid_map.insert(nodeid_suid_pair(node_id, node_suid));

                addNewNodeFromDemographics(node_suid, demographics_factory, climate_factory);
            }
        }

        if( enable_property_output && Node::base_distribs.size() == 0 )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "<Number of Individual Properties>", "0", "Enable_Property_Output", "1" );
        }

        // Merge nodeid<->suid bimaps
        nodeid_suid_map_t merged_map;
        MergeNodeIdSuidBimaps( nodeid_suid_map, merged_map );

        // Initialize migration structure from file
        IMigrationInfoFactory * migration_factory = CreateMigrationInfoFactory( idreference, 
                                                                                m_simConfigObj->migration_structure, 
                                                                                m_simConfigObj->default_torus_size );

        if (migration_factory == nullptr)
        {
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "IMigrationInfoFactory" );
        }

        can_support_family_trips = IndividualHumanConfig::CanSupportFamilyTrips( migration_factory );

        release_assert( m_simConfigObj );
        if( (m_simConfigObj->migration_structure != MigrationStructure::NO_MIGRATION) &&
            m_simConfigObj->demographics_initial &&
            !migration_factory->IsAtLeastOneTypeConfiguredForIndividuals() )
        {
            LOG_WARN("Enable_Demographics_Initial is set to true,  Migration_Model != NO_MIGRATION, and no migration file names have been defined and enabled.  No file-based migration will occur.\n");
        }

        for (auto& entry : nodes)
        {
            release_assert(entry.second);
            (entry.second)->SetupMigration( migration_factory, m_simConfigObj->migration_structure, merged_map );
        }

        LoadInterventions( campaignfilename );

#ifndef DISABLE_CLIMATE
        // Clean up
        delete climate_factory; 
        climate_factory = nullptr;
#endif

        delete migration_factory;
        migration_factory = nullptr;


        LOG_INFO_F( "populateFromDemographics() created %d nodes\n", nodes.size() );
        return int(nodes.size());
    }

    void Kernel::Simulation::addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory)
    {
        Node *node = Node::CreateNode(this, node_suid);
        addNode_internal(node, nodedemographics_factory, climate_factory);
    }

    void Kernel::Simulation::addNode_internal( INodeContext *node, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory)
    {
        release_assert(node);
        release_assert(nodedemographics_factory);
#ifndef DISABLE_CLIMATE
        release_assert(climate_factory);
#endif

        // Node initialization
        node->SetParameters(nodedemographics_factory, climate_factory);
        node->SetMonteCarloParameters(Ind_Sample_Rate);// need to define parameters

        // Populate node
        node->PopulateFromDemographics();

        // Add node to the map
        nodes.insert( std::pair<suids::suid, INodeContext*>(node->GetSuid(), node) );
        node_event_context_list.push_back( node->GetEventContext() );
        nodeRankMap.Add( EnvPtr->MPI.Rank, node );

        notifyNewNodeObservers(node);
    } 

    void Simulation::loadCampaignFromFile( const std::string& campaignfilename )
    {
        // load in the configuration
        // parse the DM creation events, create them, and add them to an event queue
        event_context_host->LoadCampaignFromFile( campaignfilename );
    }

    void Simulation::notifyNewNodeObservers(INodeContext* node)
    {
        if (new_node_observers.size() > 0)
        {
            for (const auto& entry : new_node_observers)
            {
                entry.second(node);
            }
        }
    }

    //------------------------------------------------------------------
    //   Individual migration methods
    //------------------------------------------------------------------

    void Simulation::resolveMigration()
    {
        LOG_DEBUG("resolveMigration\n");

        WithSelfFunc to_self_func = [this](int myRank) 
        { 
#ifndef CLORTON
            // Don't bother to serialize locally
            // for (auto individual : migratingIndividualQueues[destination_rank]) // Note the direction of iteration below!
            for (auto iterator = migratingIndividualQueues[myRank].rbegin(); iterator != migratingIndividualQueues[myRank].rend(); ++iterator)
            {
                auto individual = *iterator;
                auto* emigre = dynamic_cast<IMigrate*>(individual);
                emigre->ImmigrateTo( nodes[emigre->GetMigrationDestination()] );
                if( individual->IsDead() )
                {
                    // we want the individual to migrate home and then finish dieing
                    delete individual;
                }
            }
#else
            if ( migratingIndividualQueues[myRank].size() > 0 )
            {
                auto writer = make_shared<BinaryArchiveWriter>();
                (*static_cast<IArchive*>(writer.get())) & migratingIndividualQueues[myRank];

                for (auto& individual : migratingIndividualQueues[myRank])
                    delete individual; // individual->Recycle();

                migratingIndividualQueues[myRank].clear();

                //if ( EnvPtr->Log->CheckLogLevel(Logger::VALIDATION, _module) ) {
                //    _write_json( int(currentTime.time), EnvPtr->MPI.Rank, myRank, "self", static_cast<IArchive*>(writer.get())->GetBuffer(), static_cast<IArchive*>(writer.get())->GetBufferSize() );
                //}

                auto reader = make_shared<BinaryArchiveReader>(static_cast<IArchive*>(writer.get())->GetBuffer(), static_cast<IArchive*>(writer.get())->GetBufferSize());
                (*static_cast<IArchive*>(reader.get())) & migratingIndividualQueues[myRank];
                for (auto individual : migratingIndividualQueues[myRank])
                {
                    auto* immigrant = dynamic_cast<IMigrate*>(individual);
                    immigrant->ImmigrateTo( nodes[immigrant->GetMigrationDestination()] );
                }
            }
#endif
        }; 

        SendToOthersFunc to_others_func = [this](IArchive* writer, int toRank)
        {
            *writer & migratingIndividualQueues[toRank];
            for (auto& individual : migratingIndividualQueues[toRank])
                individual->Recycle();  // delete individual
        };

        ClearDataFunc clear_data_func = [this](int rank)
        {
            migratingIndividualQueues[rank].clear();
        };

        ReceiveFromOthersFunc from_others_func = [this](IArchive* reader, int fromRank)
        {
            *reader & migratingIndividualQueues[fromRank];
            for (auto individual : migratingIndividualQueues[fromRank])
            {
                IMigrate* immigrant = dynamic_cast<IMigrate*>(individual);
                immigrant->ImmigrateTo( nodes[immigrant->GetMigrationDestination()] );
                if( individual->IsDead() )
                {
                    // we want the individual to migrate home and then finish dieing
                    delete individual;
                }
            }
        };

        MpiDataExchanger exchanger( "HumanMigration", to_self_func, to_others_func, from_others_func, clear_data_func );
        exchanger.ExchangeData( this->currentTime );
    }

    void Simulation::PostMigratingIndividualHuman(IIndividualHuman *i)
    {
        migratingIndividualQueues[nodeRankMap.GetRankFromNodeSuid(i->GetMigrationDestination())].push_back(i);
    }

    bool Simulation::CanSupportFamilyTrips() const
    {
        return can_support_family_trips;
    }

    //------------------------------------------------------------------
    //   Assorted getters and setters
    //-----------------------------------------------------------------

    const DemographicsContext* Simulation::GetDemographicsContext() const
    {
        return demographicsContext;
    }

    IdmDateTime Simulation::GetSimulationTime() const
    {
        return currentTime;
    }

    int Simulation::GetSimulationTimestep() const
    {
        return currentTime.timestep;
    }

    suids::suid Simulation::GetNextNodeSuid()
    {
        return nodeSuidGenerator();
    }

    suids::suid Simulation::GetNextIndividualHumanSuid()
    {
        return individualHumanSuidGenerator();
    }

    suids::suid Simulation::GetNextInfectionSuid()
    {
        return infectionSuidGenerator();
    }

    suids::suid Simulation::GetNodeSuid( uint32_t external_node_id )
    {
        return nodeRankMap.GetSuidFromExternalID( external_node_id );
    }

    ExternalNodeId_t Simulation::GetNodeExternalID( const suids::suid& rNodeSuid )
    {
        return nodeRankMap.GetNodeInfo( rNodeSuid ).GetExternalID();
    }

    uint32_t Simulation::GetNodeRank( const suids::suid& rNodeSuid )
    {
        return nodeRankMap.GetRankFromNodeSuid( rNodeSuid );
    }

    RANDOMBASE* Simulation::GetRng()
    {
        return rng;
    }

    void Simulation::ResetRng()
    {
        setupRng();
    }

    std::vector<IReport*>& Simulation::GetReports()
    {
        return reports;
    }

    std::vector<IReport*>& Simulation::GetReportsNeedingIndividualData()
    {
        return individual_data_reports ;
    }

    int Simulation::getInitialRankFromNodeId( ExternalNodeId_t node_id )
    {
        return nodeRankMap.GetInitialRankFromNodeId(node_id); // R: leave as a wrapper call to nodeRankMap.GetInitialRankFromNodeId()
    }
    
    ISimulationContext * Simulation::GetContextPointer() 
    { 
        return this; 
    }

    void Simulation::PropagateContextToDependents()
    {
        ISimulationContext *context = GetContextPointer();
        for (auto& entry : nodes)
        {
            entry.second->SetContextTo(context);
        }
    }

    // IGlobalContext inferface for all the other components 
    const SimulationConfig* Simulation::GetSimulationConfigObj() const
    {
        return m_simConfigObj;
    }

    const IInterventionFactory* Simulation::GetInterventionFactory() const
    {
        return m_interventionFactoryObj;
    }

    REGISTER_SERIALIZABLE(Simulation);

    void Simulation::serialize(IArchive& ar, Simulation* obj)
    {
        Simulation& sim = *obj;
        ar.labelElement("serializationMask") & (uint32_t&)sim.serializationMask;

        if ((sim.serializationMask & SerializationFlags::Population) != 0) {
            ar.labelElement("nodes"); serialize(ar, sim.nodes);
        }

        if ((sim.serializationMask & SerializationFlags::Parameters) != 0) {
            ar.labelElement("campaignFilename") & sim.campaignFilename;

            ar.labelElement("Ind_Sample_Rate") & sim.Ind_Sample_Rate;

            ar.labelElement("sim_type") & (uint32_t&)sim.sim_type;
            ar.labelElement("demographic_tracking") & sim.demographic_tracking;
            ar.labelElement("enable_spatial_output") & sim.enable_spatial_output;
            ar.labelElement("enable_property_output") & sim.enable_property_output;
            ar.labelElement("enable_default_report") & sim.enable_default_report;
            ar.labelElement("enable_event_report") & sim.enable_event_report;

            ar.labelElement("loadbalance_filename") & sim.loadbalance_filename;
            ar.labelElement("Run_Number") & sim.Run_Number;
        }

        if ((sim.serializationMask & SerializationFlags::Properties) != 0) {
// clorton          ar.labelElement("nodeRankMap") & sim.nodeRankMap;
// clorton          ar.labelElement("node_event_context_list") & sim.node_event_context_list;
// clorton          ar.labelElement("nodeid_suid_map") & sim.nodeid_suid_map;
// clorton          ar.labelElement("migratingIndividualQueues") & sim.migratingIndividualQueues;
// clorton          ar.labelElement("m_simConfigObj") & sim.m_simConfigObj;
// clorton          ar.labelElement("m_interventionFactoryObj") & sim.m_interventionFactoryObj;
// clorton          ar.labelElement("demographicsContext") & sim.demographicsContext;
// clorton          ar.labelElement("infectionSuidGenerator") & sim.infectionSuidGenerator;
// clorton          ar.labelElement("individualHumanSuidGenerator") & sim.individualHumanSuidGenerator;
// clorton          ar.labelElement("nodeSuidGenerator") & sim.nodeSuidGenerator;
            ar.labelElement("loadBalanceFilename") & sim.loadBalanceFilename;
// clorton          ar.labelElement("rng") & sim.rng;
// clorton          ar.labelElement("reports") & sim.reports;
// clorton          ar.labelElement("individual_data_reports") & sim.individual_data_reports;
// clorton          ar.labelElement("reportClassCreator") & sim.reportClassCreator;
// clorton          ar.labelElement("binnedReportClassCreator") & sim.binnedReportClassCreator;
// clorton          ar.labelElement("spatialReportClassCreator") & sim.spatialReportClassCreator;
// clorton          ar.labelElement("propertiesReportClassCreator") & sim.propertiesReportClassCreator;
// clorton          ar.labelElement("demographicsReportClassCreator") & sim.demographicsReportClassCreator;
// clorton          ar.labelElement("eventReportClassCreator") & sim.eventReportClassCreator;
// clorton          ar.labelElement("event_coordinators") & sim.event_coordinators;
// clorton          ar.labelElement("campaign_events") & sim.campaign_events;
// clorton          ar.labelElement("event_context_host") & sim.event_context_host;
// clorton          ar.labelElement("currentTime") & sim.currentTime;
            ar.labelElement("random_type") & (uint32_t&)sim.random_type;
            ar.labelElement("campaign_filename") & sim.campaign_filename;
// clorton          ar.labelElement("demographics_factory") & sim.demographics_factory;
// clorton          ar.labelElement("new_node_observers") & sim.new_node_observers;
        }
    }

    void Simulation::serialize(IArchive& ar, NodeMap_t& node_map)
    {
        size_t count = (ar.IsWriter() ? node_map.size() : -1);
        ar.startArray(count);
        if (ar.IsWriter())
        {
            for (auto& entry : node_map)
            {
                ar.startObject();
                ar.labelElement("suid_data") & (uint32_t&)(entry.first.data);
                ar.labelElement("node") & entry.second;
                ar.endObject();
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                ar.startObject();
                suids::suid suid;
                ISerializable* obj;
                ar.labelElement("suid_data") & suid.data;
                ar.labelElement("node") & obj;
                ar.endObject();
                node_map[suid] = static_cast<Node*>(obj);
            }
        }
        ar.endArray();
    }

#if 0
    template<class Archive>
    void serialize(Archive & ar, Simulation &sim, const unsigned int  file_version )
    {
        LOG_DEBUG("(De)serializing Simulation\n");

        ar & sim.Ind_Sample_Rate;// Fraction of individuals in each community to sample, base rate, can be modified for each community
        // Counters
        ar & sim.currentTime;
        ar & sim.currentTimestep; // counts number of timesteps actually taken

        ar & sim.nodes;
        ar & sim.nodeRankMap; // need to preserve this to support serializing a distributed state
        ar & sim.nodeid_suid_map;

        ar & sim.demographicsContext;

        ar & sim.nodeSuidGenerator;
        ar & sim.individualHumanSuidGenerator;
        ar & sim.infectionSuidGenerator;

        ar  & sim.campaignFilename
        ar & sim.event_context_host;
        ar.register_type(static_cast<PSEUDO_DES*>(nullptr));
        ar & sim.rng;

        if (typename Archive::is_loading())
        {
            sim.PropagateContextToDependents(); // HACK: boost serialization should have been able to do this automagically but fails on abstract classes even though it shouldn't. hopefully wont have to fix later
        }
    }
#endif

}
