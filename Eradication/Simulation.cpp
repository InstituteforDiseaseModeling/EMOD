/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "Simulation.h"

#include <ctime>
#include <iomanip>      // std::setprecision

#ifdef WIN32
#include "windows.h"
#endif

#include "FileSystem.h"
#include "Debug.h"
#include "Report.h"
#include "BinnedReport.h"
#include "DemographicsReport.h"
#include "SpatialReport.h"
#include "PropertyReport.h"
#include "Exceptions.h"
#include "IndividualEventContext.h"
#include "Instrumentation.h"
#include "InterventionEnums.h"
#include "Migration.h"
#include "Node.h"
#include "NodeDemographics.h"
#include "ProgVersion.h" // Version info extraction for both program and dlls
#include "RANDOM.h"
#include "SimulationConfig.h"
#include "SimulationEventContext.h"
#include "StatusReporter.h"
#include "ReportEventRecorder.h"

#include "DllLoader.h"

#include "RapidJsonImpl.h"

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
        : demographicsContext(nullptr)
        , nodeSuidGenerator(EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks)
        , individualHumanSuidGenerator(EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks)
        , infectionSuidGenerator(EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks)
        , rng(nullptr)
        , event_context_host(nullptr)
        , reportClassCreator(nullptr)
        , propertiesReportClassCreator(nullptr)
        , demographicsReportClassCreator(nullptr)
        , eventReportClassCreator(nullptr)
        , m_simConfigObj(nullptr)
        , m_interventionFactoryObj(nullptr)
        , new_node_observers()
        , random_type(RandomType::USE_PSEUDO_DES)
        , demographic_tracking(false)
        , enable_event_report(false)
        , enable_spatial_output(false)
        , enable_property_output(false)
        , enable_default_report(false)
        , campaign_filename()
        , loadbalance_filename()
        , Run_Number(0)
        , demographics_factory(nullptr)
    {
        LOG_DEBUG( "CTOR\n" );

        reportClassCreator              = Report::CreateReport;
        binnedReportClassCreator        = BinnedReport::CreateReport;
        spatialReportClassCreator       = SpatialReport::CreateReport;
        propertiesReportClassCreator    = PropertyReport::CreateReport;
        demographicsReportClassCreator  = DemographicsReport::CreateReport;
        eventReportClassCreator         = ReportEventRecorder::CreateReport;

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
        event_context_host = NULL;

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
        Simulation *newsimulation = NULL;

        newsimulation = _new_ Simulation();
        if (newsimulation)
        {
            // This sequence is important: first
            // Creation-->Initialization-->Validation
            newsimulation->Initialize(config);
            if(!ValidateConfiguration(config))
            {
                delete newsimulation;
                newsimulation = NULL;
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
            randomseed[0] = (uint16_t) Run_Number;
            randomseed[1] = (uint16_t) EnvPtr->MPI.Rank;
            rng = _new_ PSEUDO_DES(*((uint32_t*) randomseed));
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
                                ((int)(p_cr_config->operator[]( "Use_Explicit_Dlls" ).As<json::Number>()) != 1) ;

        LOG_INFO_F("Found %d Custom Report DLL's to consider loading\n", rReportInstantiatorMap.size() );
        for( auto ri_entry : rReportInstantiatorMap )
        {
            std::string class_name = ri_entry.first ;
            try
            {
                if( (p_cr_config != nullptr) && p_cr_config->Exist( class_name ) )
                {
                    LOG_INFO_F("Found custom report data for %s\n", class_name.c_str());
                    json::QuickInterpreter dll_data = p_cr_config->operator[]( class_name ).As<json::Object>() ;
                    if( (int)(dll_data["Enabled"].As<json::Number>()) != 0 )
                    {
                        json::Array report_data = dll_data["Reports"].As<json::Array>() ;
                        for( int i = 0 ; i < report_data.Size() ; i++ )
                        {
                            Configuration* p_cfg = Configuration::CopyFromElement( report_data[i] );

                            IReport* p_cr = ri_entry.second(); // creates report object
                            p_cr->Configure( p_cfg );
                            reports.push_back( p_cr );
                            delete p_cfg ;
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
                }
            }
            catch( json::Exception& e )
            {
                std::stringstream ss ;
                ss << "Error occured reading report data for " << class_name << ".  Error: " << e.what() << std::endl ;
                throw InitializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
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

    void Simulation::Reports_LogNodeData( Node* n )
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
            Node *n = entry.second;
            stat_pop += n->GetStatPop();
            infected += n->GetInfected();
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << "Update(): Time: " << (float)currentTime.time;
        if( currentTime._base_year > 0 )
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
        for (auto iterator = nodes.rbegin(); iterator != nodes.rend(); iterator++)
        {
            Node *n = iterator->second;
            release_assert(n);
            n->Update(dt);

            Reports_LogNodeData( n );
        }

        // -----------------------
        // --- Resolve Migration
        // -----------------------
        REPORT_TIME( ENABLE_DEBUG_MPI_TIMING, "resolveMigration", resolveMigration() );

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

    struct Simulation::map_merge : public std::binary_function<nodeid_suid_map_t, nodeid_suid_map_t, nodeid_suid_map_t>
    {
        struct merge_duplicate_key_exception : public std::exception
        {
            virtual const char* what() const throw() { return "Duplicate key in map merge\n"; }
        };

        nodeid_suid_map_t operator()(const nodeid_suid_map_t& x, const nodeid_suid_map_t& y) const
        {
            nodeid_suid_map_t mergedMap;

            for (auto& pair : x)
            {
                if(!(mergedMap.insert(pair).second))
                    throw(merge_duplicate_key_exception());
            }

            for (auto& pair : y)
            {
                if(!(mergedMap.insert(pair).second)) // .second is false if the key already existed
                    throw(merge_duplicate_key_exception());
            }

            return mergedMap;
        }
    };

    int Simulation::populateFromDemographics(const char* campaignfilename, const char* loadbalancefilename)
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
        }

        JsonConfigurable::_track_missing = true;

        // Initialize node demographics from file
        demographics_factory = NodeDemographicsFactory::CreateNodeDemographicsFactory(&nodeid_suid_map, EnvPtr->Config);
        if (demographics_factory == nullptr)
        {
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "Failed to create NodeDemographicsFactory" );
        }

        demographicsContext = demographics_factory->CreateDemographicsContext();
        string idreference  = demographics_factory->GetIdReference();
        vector<uint32_t> nodeIDs = demographics_factory->GetNodeIDs();
        ClimateFactory * climate_factory = NULL;
#ifndef DISABLE_CLIMATE
        // Initialize climate from file
        climate_factory = ClimateFactory::CreateClimateFactory(&nodeid_suid_map, EnvPtr->Config, idreference);
        if (climate_factory == NULL)
        {
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "Failed to create ClimateFactory" );
        }
#endif
        // We can validate climate structure against sim_type now.

        // Initialize load-balancing scheme from file
        boost::scoped_ptr<LegacyFileInitialLoadBalanceScheme> filescheme(_new_ LegacyFileInitialLoadBalanceScheme());
        boost::scoped_ptr<CheckerboardInitialLoadBalanceScheme> checkerboardscheme(_new_ CheckerboardInitialLoadBalanceScheme());
        if (filescheme->Initialize(loadbalancefilename, nodeIDs.size()))
        {
            LOG_INFO("Loaded load balancing file.\n");
            nodeRankMap.SetInitialLoadBalanceScheme((IInitialLoadBalanceScheme*)filescheme.get());
        }
        else 
        {
            LOG_WARN("Failed to use legacy loadbalance file. Defaulting to checkerboard.\n");
            nodeRankMap.SetInitialLoadBalanceScheme((IInitialLoadBalanceScheme*)checkerboardscheme.get());
        }

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
        MPI_Barrier( MPI_COMM_WORLD );

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
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        merged_map = all_reduce(*(EnvPtr->MPI.World), nodeid_suid_map, map_merge());
#else
        merged_map = nodeid_suid_map;
#endif

        // Initialize migration structure from file
        MigrationInfoFactory * migration_factory = NULL;
        release_assert( m_simConfigObj );
        if (m_simConfigObj->migration_structure != MigrationStructure::NO_MIGRATION)
        {
            migration_factory = MigrationInfoFactory::CreateMigrationInfoFactory(&merged_map, EnvPtr->Config, idreference);

            if (migration_factory == NULL)
            {
                throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "MigrationInfoFactory" );
            }

            if (m_simConfigObj->demographics_initial          &&
                migration_factory->airmig_filename.empty()    &&
                migration_factory->localmig_filename.empty()  &&
                migration_factory->regionmig_filename.empty() &&
                migration_factory->seamig_filename.empty())
            {
                throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Enable_Demographics_Initial and Migration_Model", "true and not NO_MIGRATION (respectively)", "(all of air, local, regional, and sea migration filenames)", "(empty)");
            }

            for (auto& entry : nodes)
            {
                release_assert(entry.second);
                (entry.second)->SetupMigration(migration_factory);
            }
        }

#ifndef DISABLE_CLIMATE
        // Clean up
        delete climate_factory; 
        climate_factory = NULL;
#endif

        delete migration_factory;
        migration_factory = NULL;


        LOG_INFO_F( "populateFromDemographics() created %d nodes\n", nodes.size() );
        return (int)nodes.size();
    }

    void Kernel::Simulation::addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory)
    {
        Node *node = Node::CreateNode(this, node_suid);
        addNode_internal(node, nodedemographics_factory, climate_factory);
    }

    void Kernel::Simulation::addNode_internal(Node *node, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory)
    {
        release_assert(node);
        release_assert(nodedemographics_factory);
#ifndef DISABLE_CLIMATE
        release_assert(climate_factory);
#endif

        // Add node to the map
        nodes.insert(std::pair<suids::suid, Node*>(node->GetSuid(), (Node*)node));
        node_event_context_list.push_back( node->GetEventContext() );
        nodeRankMap.Add(node->GetSuid(), EnvPtr->MPI.Rank);

        // Node initialization
        node->SetParameters(nodedemographics_factory, climate_factory);
        node->SetMonteCarloParameters(Ind_Sample_Rate);// need to define parameters

        // Populate node
        node->PopulateFromDemographics();

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

        // Entire implementation of "resolveMigrationInternal" moved to header file
        // as a (necessary?) step towards disease-specific DLLs (because of the template)
        resolveMigrationInternal(typed_migration_queue_storage, migratingIndividualQueues);
    }

    void Simulation::PostMigratingIndividualHuman(IndividualHuman *i)
    {
        migratingIndividualQueues[nodeRankMap.GetRankFromNodeSuid(i->GetMigrationDestination())].push_back(i);
    }

    boost::mpi::request Simulation::sendHuman(IndividualHuman *ind_human, int dest_rank)
    {
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        boost::mpi::request request = EnvPtr->MPI.World->isend(dest_rank, 0, *ind_human);
#else
        boost::mpi::request request;
#endif
        return request;
    }

    IndividualHuman* Simulation::receiveHuman(int src_rank)
    {
        IndividualHuman* newHuman = IndividualHuman::CreateHuman();
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        boost::mpi::request request   = EnvPtr->MPI.World->irecv(src_rank, 0, *newHuman); 
        request.wait();
#endif

        // TODO: handle errors by returning NULL
        return newHuman;
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

    int Simulation::getInitialRankFromNodeId(node_id_t node_id)
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

#if USE_BOOST_SERIALIZATION
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
        ar.register_type(static_cast<PSEUDO_DES*>(NULL));
        ar & sim.rng;

        if (typename Archive::is_loading())
        {
            sim.PropagateContextToDependents(); // HACK: boost serialization should have been able to do this automagically but fails on abstract classes even though it shouldn't. hopefully wont have to fix later
        }
    }
#endif


    void
    Simulation::doJsonDeserializeReceive()
    {

        for (int src_rank = 0; src_rank < EnvPtr->MPI.NumTasks; src_rank++)
        {
            if (src_rank == EnvPtr->MPI.Rank) continue; // we'll never receive a message from our own process

            int receiving_humans;
            MPI_Status status;
            MPI_Request mpi_request;
            MPI_Irecv(&receiving_humans, 1, MPI_INT, src_rank, 0, MPI_COMM_WORLD, &mpi_request); // receive number of humans I need to receive
            MPI_Wait(&mpi_request, &status);
            LOG_DEBUG_F( "mpi receiving %d humans from src_rank=%d\n", receiving_humans, src_rank );
            if (receiving_humans > 0) 
            {
                //if (ENABLE_DEBUG_MPI_TIMING)
                //    LOG_INFO_F("syncDistributedState(): Rank %d receiving %d individuals from %d.\n", EnvPtr->MPI.Rank, receiving_humans, src_rank);

                // receive the whole list structure
                //typed_migration_queue_storage.receive_queue.resize(receiving_humans);

                LOG_DEBUG( "rapidjson receiving!\n" );
                std::string receive_buffer;
                boost::mpi::request request = EnvPtr->MPI.World->irecv(src_rank, 0, receive_buffer);                 

                request.wait();
                
                LOG_DEBUG( "rapidjson Parsing!\n" );
                std::list< IndividualHuman* > immigrants;
                try
                {
                    //json::Reader::Read( serialPplJsonArray, serialPpl );
                    rapidjson::Document document;
                    document.Parse<0>( receive_buffer.c_str() );
                    LOG_DEBUG( "rapidjson Parsed!\n" );
                    LOG_DEBUG_F( "Creating & deserializing %d humans.\n", document.Size() );
                    //for( unsigned int idx = 0; idx < ppl_array.Size(); idx ++ )
                    bool infected_migrant = false;
                    for( unsigned int idx = 0; idx < document.Size(); idx ++ )
                    {

                        // Check if this is base class IndividualHuman, or the derived one
                        int mig_dest_data = -1;
                        if (document[ idx ]["IndividualHuman"].IsObject())
                        {
                            // derived class
                            mig_dest_data = (int) document[ idx ]["IndividualHuman"][ "migration_destination" ][ "data" ].GetInt();
                        }
                        else
                        {
                            // it is base class
                            mig_dest_data = (int) document[ idx ][ "migration_destination" ][ "data" ].GetInt();
                        }
                       
                        suids::suid node_id;
                        //LOG_DEBUG( "Getting migration_destination\n" );
                        node_id.data = mig_dest_data;
                        Node * targetNode = nodes[ node_id ];
                        if( targetNode == NULL )
                        {
                            LOG_WARN( "Weird, node pointer is null. We get sent to the wrong place????\n" );
                            continue;
                        }
                        //IndividualHuman * new_individual = targetNode->addNewIndividual(0.0/*mc weight*/, 0.0 /*age*/, 0, 0 /*(int)(rand()%20)/18.0*//*infs*/, (float)0.0 /*imm*/, (float)0.0/*risk*/, (float)0.0f/*mighet*/, (float)0.0f /*poverty*/);
                        IndividualHuman * new_individual = targetNode->addNewIndividualFromSerialization();

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
                        new_individual->JDeserialize( (IJsonObjectAdapter*) &document[ idx ], NULL );
                        //new_individual->JDeserialize( json::json_cast< const json::Object& > ( ppl_array[ idx ] ) );
#endif

                        LOG_DEBUG_F( "new_individual ImmigrateTo to targetNode: %d\n", node_id.data );
                        new_individual->ImmigrateTo( targetNode ); // this causes crashes for some inexplicable reason: memory corruption???


                        //infected_migrant |= ( document[ idx ][ "infections" ].Size() > 0 ? true : false );
                    }

                    //LOG_INFO_F("Receive from src rank %d humans %d for %d bytes\n", src_rank, receiving_humans, receive_buffer.size()); 
#if 0
                    if( infected_migrant )
                    {
                        std::ostringstream msgFileName;
                        msgFileName << "message_received" << EnvPtr->MPI.Rank << ".json";

                        std::ofstream msgReceived( msgFileName.str(), std::ios::out );
                        msgReceived << receive_buffer;
                        msgReceived.close();
                    }
#endif

                }
                catch( json::Exception &e )
                {
                    throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() );
                }

                //typed_migration_queue_storage.receive_queue.clear();

            } // if (receiving_humans > 0) 

        } // for (int src_rank = 0; src_rank < EnvPtr->MPI.NumTasks; src_rank++)
        LOG_DEBUG( "Done receiving.\n" );

    } 
}


#if USE_BOOST_SERIALIZATION
template void Kernel::serialize( boost::archive::binary_oarchive & ar, Simulation& sim, const unsigned int file_version);
template void Kernel::serialize( boost::archive::binary_iarchive & ar, Simulation& sim, const unsigned int file_version);
#endif

#ifdef _DLLS_
// hack because of weird build error on dll, just seeing if this works, will use as data to solve problem
//Environment* EnvPtr = NULL;
#endif
