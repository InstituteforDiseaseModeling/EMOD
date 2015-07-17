/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <functional>
#include <list>
#include <map>
#include <unordered_map>
#include <string>
#include "suids.hpp"

#include "BoostLibWrapper.h"
#include "Contexts.h"
#include "Environment.h"
#include "IdmDateTime.h"
#include "Individual.h"
#include "Infection.h"
#include "ISimulation.h"
#include "Node.h"
#include "NodeRankMap.h"
#include "IReport.h"
#include "Susceptibility.h"
#include "Configure.h"
#include "IdmApi.h"

class RANDOMBASE;

#define ENABLE_DEBUG_MPI_TIMING 0  // TODO: could make this an environment setting so we don't have to recompile

namespace Kernel
{
    class  CampaignEvent;
    struct IEventCoordinator;
    struct SimulationEventContext;
    class  SimulationEventContextHost;

    typedef uint32_t node_id_t;

    class IDMAPI Simulation : public ISimulation, public ISimulationContext, public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(Simulation)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:

        static Simulation *CreateSimulation();  // for serialization
        static Simulation *CreateSimulation(const ::Configuration *config);
        virtual ~Simulation();

        virtual bool Configure( const ::Configuration *json );

        // IGlobalContext interfaces
        virtual const SimulationConfig* GetSimulationConfigObj() const;
        virtual const IInterventionFactory* GetInterventionFactory() const;

        // ISimulation methods
        virtual bool  Populate();
        virtual void  Update(float dt);
        virtual int   GetSimulationTimestep() const;
        virtual IdmDateTime GetSimulationTime() const;
        virtual void  RegisterNewNodeObserver(void* id, Kernel::ISimulation::callback_t observer);
        virtual void  UnregisterNewNodeObserver(void* id);
        virtual void  WriteReportsData();

        virtual const DemographicsContext* GetDemographicsContext() const;

        // Migration
        virtual void PostMigratingIndividualHuman(IndividualHuman *i);

        // Unique ID services
        virtual suids::suid GetNextNodeSuid();
        virtual suids::suid GetNextIndividualHumanSuid();
        virtual suids::suid GetNextInfectionSuid();

        // Random number handling
        virtual RANDOMBASE* GetRng();
        void ResetRng(); // resets random seed to a new value. necessary when branching from a common state

        // Reporting
        virtual std::vector<IReport*>& GetReports();
        virtual std::vector<IReport*>& GetReportsNeedingIndividualData();


    protected:

        Simulation();
        virtual void Initialize();  // for serialization
        virtual void Initialize(const ::Configuration *config);

        static bool ValidateConfiguration(const ::Configuration *config);

        virtual void Reports_CreateBuiltIn();
        virtual void Reports_CreateCustom();

        // Initialization
        virtual void setupEventContextHost();
        virtual void setupMigrationQueues();
        void setupRng();
        void setParams( const ::Configuration *config );
        void initSimulationState();

        // TODO: this is only here temporarily... should really go in a MigrationFactory class or 
        // something similar to ClimateFactory::ParseMetadataForFile() when migration gets refactored
        bool ParseMetadataForMigrationFile(std::string data_filepath, std::string idreference, hash_map<uint32_t, uint32_t> &node_offsets);

        // Node initialization
        int  populateFromDemographics(const char* campaign_filename, const char* loadbalance_filename); // creates nodes from demographics input file data
        virtual void addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory); // For derived Simulation classes to add correct node type
        void addNode_internal(Node *node, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory); // Helper to add Nodes
        int  getInitialRankFromNodeId(node_id_t node_id); // Needed in MPI implementation

        // Migration
        boost::mpi::request sendHuman(IndividualHuman *ind_human, int dest_rank);
        IndividualHuman*    receiveHuman(int src_rank);
        virtual void resolveMigration(); // derived classes overide this...

        // Campaign input file parsing
        virtual void   loadCampaignFromFile( const std::string& campaign_filename );

        virtual void notifyNewNodeObservers(INodeContext*);

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        // Nodes
        typedef std::map< suids::suid, Node* > NodeMap_t; // TODO: change to unordered_map for better asymptotic performance
        typedef NodeMap_t::value_type NodeMapEntry_t;
        NodeMap_t nodes;
        NodeRankMap nodeRankMap;

        std::vector<INodeEventContext*> node_event_context_list ;

        typedef boost::bimap<uint32_t, suids::suid> nodeid_suid_map_t;
        typedef nodeid_suid_map_t::value_type nodeid_suid_pair;
        nodeid_suid_map_t nodeid_suid_map;

        struct map_merge;

        // Migration
        std::vector< std::vector< IMigrate* > > migratingIndividualQueues;

        // Master copies of contained-class flags are maintained here so that they only get serialized once
        // TODO: deprecate and use SimulationConfig everywhere
        const SimulationConfig*     m_simConfigObj;
        const IInterventionFactory* m_interventionFactoryObj;
        const DemographicsContext *demographicsContext;

        // Simulation-unique ID generators for each type of child object that might exist in our system
        suids::distributed_generator<Infection> infectionSuidGenerator;
        suids::distributed_generator<IndividualHuman> individualHumanSuidGenerator;
        suids::distributed_generator<Node> nodeSuidGenerator;

        // Input files
        std::string campaignFilename;
        std::string loadBalanceFilename;

        // RNG services
        RANDOMBASE* rng;

        // Reporting
        std::vector<IReport*> reports;                          // Reporter container
        std::vector<IReport*> individual_data_reports;          // subset of 'reports' vector

        typedef IReport* (*tReportClassCreator)();
        tReportClassCreator reportClassCreator;                 // Reporting class factory/creator function pointer.
        tReportClassCreator binnedReportClassCreator;
        tReportClassCreator spatialReportClassCreator;
        tReportClassCreator propertiesReportClassCreator;
        tReportClassCreator demographicsReportClassCreator;
        tReportClassCreator eventReportClassCreator;

        // Coordination of events for campaign intervention events
        std::list<IEventCoordinator*> event_coordinators;
        std::list<CampaignEvent*>     campaign_events;

        friend class SimulationEventContextHost;
        friend class Node;
        SimulationEventContextHost *event_context_host;

        // Parameters
        float Ind_Sample_Rate; // Fraction of individuals in each node to sample; can be modified for each community

        // Counters
        IdmDateTime currentTime;

        // JsonConfigurable variables
        RandomType::Enum                                    random_type;                                // RANDOM_TYPE
        SimType::Enum                                        sim_type;                                    // Sim_Type
        bool demographic_tracking;
        bool enable_spatial_output;
        bool enable_property_output;
        bool enable_default_report ;
        bool enable_event_report;
        std::string campaign_filename;
        std::string loadbalance_filename;
        int Run_Number;

        NodeDemographicsFactory* demographics_factory;
#pragma warning( pop )
    protected:

        //------------------------------------------------------------------------------------
        // N.B. Entire implementation of "resolveMigrationInternal" appears here in header file
        //      as a (necessary?) step towards disease-specific DLLs (because of the template)
        //------------------------------------------------------------------------------------

        void doJsonDeserializeReceive();

        // Helper class to minimize code duplication in derived classes
        template <class IndividualT>
        class TypedPrivateMigrationQueueStorage
        {
        public:
            std::vector< IndividualT* > receive_queue;
            std::vector< std::vector< IndividualT* > > send_queue;

            TypedPrivateMigrationQueueStorage() { init(); }

            virtual void init()
            {
                send_queue.resize(EnvPtr->MPI.NumTasks);
                receive_queue.resize(EnvPtr->MPI.NumTasks);
            }
        };

        // ...and call this with the individual types they desire along with the appropriately typed queue storage classes
        template <class IndividualT> 
        void resolveMigrationInternal( 
            TypedPrivateMigrationQueueStorage<IndividualT> &typed_migration_queue_storage,
            std::vector< std::vector< IMigrate* > > &migrating_queues
            )
        {
            // relatively inefficient but straightforward sync step

            // TODO: investigate allgather (or whatever) to sync up numbers of individuals being sent and received

            // Create a vector of individual counts to be sent. This is necessary because
            // I cannot reuse the same memory location in subsequent calls to MPI_Isend
            // (and MPI_Bsend incurs even uglier syntax)
            // (but still hack-y and possibly slower than it needs to be)

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
            IJsonObjectAdapter* pIJsonObj = CreateJsonObjAdapter();
            std::vector<std::string*> jsstr_buffer;
#endif

            individual_count_send_buffers.resize(EnvPtr->MPI.NumTasks); // ensure cached send count buffer has enough storage

            for (int dest_rank = 0; dest_rank < EnvPtr->MPI.NumTasks; dest_rank++)
            {
                if (dest_rank == EnvPtr->MPI.Rank) 
                {
                    // distribute individuals in the queue locally 
                    for (auto iterator = migrating_queues[dest_rank].rbegin(); iterator != migrating_queues[dest_rank].rend(); iterator++)
                    {
                        auto migrant = *iterator;
                        migrant->ImmigrateTo( nodes[migrant->GetMigrationDestination()] );
                    }
                    migrating_queues[dest_rank].clear(); // clear the queue of locally migrating individuals so they aren't cleaned up later
                }
                else
                {
                    // send number of individuals going to this processor in a message. possibly not most efficient communication scheme

                    MPI_Request request;
                    individual_count_send_buffers[dest_rank] = (int)migrating_queues[dest_rank].size(); 
                    MPI_Isend(&individual_count_send_buffers[dest_rank], 1, MPI_INT, dest_rank, 0, MPI_COMM_WORLD, &request); 
                    pending_sends_plain.push_back(request);

                    // Use of Isend here presents a problem because I can't wait for the matching receive to be posted. 
                    // Provide different memory locations for each individual count. These values must remain valid until the barrier!

                    if (individual_count_send_buffers[dest_rank] > 0) 
                    {
                        //if (ENABLE_DEBUG_MPI_TIMING)
                        //    LOG_INFO_F("syncDistributedState(): Rank %d sending %d individuals to %d.\n", EnvPtr->MPI.Rank, individual_count_send_buffers[dest_rank], dest_rank);

                        // apparently I cannot send my array without casting the pointers in it first.
                        // utilize some cached class local storage to accomplish this
                        typed_migration_queue_storage.send_queue[dest_rank].resize(migrating_queues[dest_rank].size());
                        for (int k = 0; k < typed_migration_queue_storage.send_queue[dest_rank].size(); k++)
                        {
                            // static_cast appears crucial for the object to be sent whole and unperturbed. not sure why, I thought the pointer to the derived class started at the same offset as the base. but it appears not
                            typed_migration_queue_storage.send_queue[dest_rank][k] = static_cast<IndividualT*>(migrating_queues[dest_rank][k]);
                        }

#if (USE_BOOST_SERIALIZATION || USE_BOOST_MPI) && !USE_JSON_MPI
                        pending_sends.push_back(
                            EnvPtr->MPI.World->isend(dest_rank, 0, typed_migration_queue_storage.send_queue[dest_rank])
                            );
                        //LOG_DEBUG_F("sending %d individuals to %d.\n", individual_count_send_buffers[dest_rank], dest_rank);

#elif USE_JSON_SERIALIZATION || USE_JSON_MPI
                                                // Send json version of individuals.

//#define WRITEOUT
#ifdef WRITEOUT
                        ofstream ofs;
                        ostringstream stringStream;
                        static int ws = 0;
                        stringStream << "mpindiv" << currentTimestep << "_" << ws++ << ".json";
                        ofs.open(stringStream.str(), ios_base::app);
                        if (!ofs.is_open())
                        {
                            LOG_ERR_F("Failed to open file %s in serialization\n", stringStream.str());
                            return;
                        }
#endif
                        int counter = 0;
                        JSerializer js;
                        const char* sHumans;
                      

                        pIJsonObj->CreateNewWriter();
                        
                        pIJsonObj->BeginArray();
                        for (auto individual : typed_migration_queue_storage.send_queue[dest_rank])
                        {
                            // TBD: fix it later to make sure all possible individual classes have implementation
                            IJsonSerializable * jsonser_individual = nullptr;
                            // Ideally would throw QI exception but in interests of performance...
                            individual->QueryInterface( GET_IID(IJsonSerializable), (void**)&jsonser_individual );
                            //release_assert( jsonser_individual );
                            jsonser_individual->JSerialize(pIJsonObj, &js);
                        }
                        pIJsonObj->EndArray();
                        js.GetFormattedOutput(pIJsonObj, sHumans);
                            
#ifdef WRITEOUT
                        char* prettyHumans = NULL;
                        js.GetPrettyFormattedOutput(pIJsonObj, prettyHumans);
                        if (prettyHumans)
                        {
                            ofs << prettyHumans << endl;
                        }
                        else
                        {
                            LOG_ERR("failed to get prettyHumans\n");
                        }
                        if (ofs.is_open())
                            ofs.close();
#endif

                        string* stringHumans = new string(sHumans);
                        jsstr_buffer.push_back(stringHumans);
                        pending_sends.push_back(
                            //EnvPtr->MPI.World->isend(dest_rank, 0, typed_migration_queue_storage.send_queue[dest_rank])
                            EnvPtr->MPI.World->isend( dest_rank, 0, *stringHumans )
                            );

                        //LOG_INFO_F("Send to dst rank %d humans %d for %d bytes. \n", dest_rank, individual_count_send_buffers[dest_rank], strlen(sHumans));
                        pIJsonObj->FinishWriter();
#endif
                    }
                }
            }
            // Done sending

#if (USE_BOOST_SERIALIZATION || USE_BOOST_MPI) && !USE_JSON_MPI
            
            for (int src_rank = 0; src_rank < EnvPtr->MPI.NumTasks; src_rank++)
            {
                if (src_rank == EnvPtr->MPI.Rank) continue; // we'll never receive a message from our own process

                int receiving_humans = 0;
                MPI_Status status;
                MPI_Request request;

                // Non-blocking receive
                MPI_Irecv(&receiving_humans, 1, MPI_INT, src_rank, 0, MPI_COMM_WORLD, &request); // receive number of humans I need to receive
                
                // After issue the receiving command for number of humans, synchronous wait until the last MPI request (MPI_Irecv) to complete
                MPI_Wait(&request, &status);
                
                //LOG_DEBUG_F("receiving_humans = %d from src=%d\n", receiving_humans, src_rank);
                if (receiving_humans > 0) 
                {
                    //if (ENABLE_DEBUG_MPI_TIMING)
                    //    LOG_INFO_F("syncDistributedState(): Rank %d receiving %d individuals from %d.\n", EnvPtr->MPI.Rank, receiving_humans, src_rank);

                    // receive the whole list structure
                    typed_migration_queue_storage.receive_queue.resize(receiving_humans);

                    boost::mpi::request request = EnvPtr->MPI.World->irecv(src_rank, 0, typed_migration_queue_storage.receive_queue); 

                    request.wait();
                    for (auto migrant : typed_migration_queue_storage.receive_queue)
                    {
                        migrant->ImmigrateTo( nodes[migrant->GetMigrationDestination()] );
                        // TODO: error handling if nodes[dest] not present on this rank
                    }

                    typed_migration_queue_storage.receive_queue.clear();
                    //std::cout << "migrating_humans = " << receiving_humans << std::endl;
                }
            }

#elif USE_JSON_SERIALIZATION || USE_JSON_MPI
            doJsonDeserializeReceive();
#endif
            
            // Cleanup phase
            // wait for any still pending sends, then free memory for individuals that were sent
            boost::mpi::wait_all(pending_sends.begin(), pending_sends.end());
            for (auto request : pending_sends_plain)    // important - gotta free em all!
            {
                MPI_Status status;
                MPI_Wait(&request, &status);
            }

            for (int dest_rank = 0; dest_rank < EnvPtr->MPI.NumTasks; dest_rank++)
            {
                for (auto migrant : migrating_queues[dest_rank])
                {
                    delete migrant;
                }

                migrating_queues[dest_rank].clear();
                typed_migration_queue_storage.send_queue[dest_rank].clear();
            }

            pending_sends.clear();
            pending_sends_plain.clear();

            EnvPtr->MPI.World->barrier();

#if USE_JSON_SERIALIZATION || USE_JSON_MPI

            if (pIJsonObj)
            {
                delete pIJsonObj;
            }
            for (auto pstring : jsstr_buffer)
            {
                delete pstring;
            }
#endif
        }

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        map<void*, Kernel::ISimulation::callback_t> new_node_observers;
#pragma warning( pop )

    private:
        typedef std::unordered_map< std::string, report_instantiator_function_t > ReportInstantiatorMap ;
        void Reports_ConfigureBuiltIn();
        void Reports_FindReportsCollectingIndividualData( float currentTime, float dt );
        Configuration* Reports_GetCustomReportConfiguration();
        void Reports_Instantiate( ReportInstantiatorMap& rReportInstantiatorMap );
        void Reports_UpdateEventRegistration( float _currentTime, float dt );
        void Reports_BeginTimestep();
        void Reports_EndTimestep( float _currentTime, float dt );
        void Reports_LogNodeData( Node* n );
        void PrintTimeAndPopulation();

        // Handling of passing "contexts" down to nodes, individuals, etc.
        virtual ISimulationContext *GetContextPointer();
        virtual void PropagateContextToDependents();

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        // non-persistent, cached memory for optimizations for resolveMigration
        std::vector<boost::mpi::request >    pending_sends;
        std::vector<MPI_Request         >    pending_sends_plain;

        // Create a vector of individual counts to be sent. 
        // This is necessary because I cannot reuse the same memory location
        // in subsequent calls to MPI_Isend (and MPI_Bsend incurs even uglier syntax) 
        // (but still hack-y and possibly slower than it needs to be)
        std::vector<int> individual_count_send_buffers;

        // Derived classes need to implement their own member of this type and pass it to resolveMigrationWithTypedStorage.
        // Its constructor will be called from the simulation constructor and all will be happy.
        TypedPrivateMigrationQueueStorage<IndividualHuman> typed_migration_queue_storage; 
#pragma warning( pop )

#if USE_BOOST_SERIALIZATION
    private: // Serialization
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, Simulation &sim, const unsigned int /* file_version */);
#endif
    };
}

#if USE_BOOST_SERIALIZATION
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Kernel::ISimulationContext)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Kernel::SimulationEventContextHost) // ???
#endif
