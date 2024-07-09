
#pragma once
#include "Simulation.h"
#include "IIdGeneratorSTI.h"
#include "ISTISimulationContext.h"
#include "Sugar.h" // for DECLARE_VIRTUAL_BASE

namespace Kernel
{
    class IndividualHumanSTI;
    struct IRelationship;
    struct INodeSTI;

    class SimulationSTI : public Simulation, public IIdGeneratorSTI, public ISTISimulationContext
    {
        GET_SCHEMA_STATIC_WRAPPER(SimulationSTI)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        virtual ~SimulationSTI(void);
        static SimulationSTI *CreateSimulation();
        static SimulationSTI *CreateSimulation(const ::Configuration *config);
        SimulationSTI();

        // methods of IIdGeneratorSTI
        virtual suids::suid GetNextRelationshipSuid() override;

        // methods of ISTISimulationContext
        virtual void AddTerminatedRelationship( const suids::suid& nodeSuid, const suids::suid& relId ) override;
        virtual bool WasRelationshipTerminatedLastTimestep( const suids::suid& relId ) const override;
        virtual suids::suid GetNextCoitalActSuid() override;
        virtual void AddEmigratingRelationship( IRelationship* pRel ) override;

        // INodeInfoFactory
        virtual INodeInfo* CreateNodeInfo() override;
        virtual INodeInfo* CreateNodeInfo( int rank, INodeContext* pNC ) override;

    protected:

        virtual void Initialize() override;
        virtual void Initialize(const ::Configuration *config) override;
        virtual bool Configure( const ::Configuration *json );
        virtual void Reports_CreateBuiltIn();
        virtual void resolveMigration() override;
        virtual void setupMigrationQueues() override;

        static bool ValidateConfiguration(const ::Configuration *config);

        // Allows correct type of Node to be added by classes derived from Simulation
        virtual void addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                 suids::suid node_suid,
                                                 NodeDemographicsFactory *nodedemographics_factory,
                                                 ClimateFactory *climate_factory ) override;
        virtual int populateFromDemographics( const char* campaignfilename, const char* loadbalancefilename ) override;

        suids::distributed_generator relationshipSuidGenerator;
        suids::distributed_generator coitalActSuidGenerator;

        bool report_relationship_start;
        bool report_relationship_end;
        bool report_relationship_consummated;
        bool report_transmission;
        std::map< suids::suid, INodeSTI* > nodes_sti;
        std::vector<std::vector<IRelationship*>> migrating_relationships;
        std::vector<std::map<suids::suid, IRelationship*>> migrating_relationships_vecmap;
    };
}
