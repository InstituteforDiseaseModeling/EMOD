/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "Simulation.h"
#include "IndividualSTI.h"
#include "Sugar.h" // for DECLARE_VIRTUAL_BASE

namespace Kernel
{
    class SimulationSTI : public Simulation
    {
        GET_SCHEMA_STATIC_WRAPPER(SimulationSTI)
    public:
        virtual ~SimulationSTI(void);
        static SimulationSTI *CreateSimulation();
        static SimulationSTI *CreateSimulation(const ::Configuration *config);
        SimulationSTI();
        virtual suids::suid GetNextRelationshipSuid();

    protected:

        virtual void Initialize();
        virtual void Initialize(const ::Configuration *config);
        virtual bool Configure( const ::Configuration *json );
        virtual void Reports_CreateBuiltIn();

        static bool ValidateConfiguration(const ::Configuration *config);

        // Allows correct type of Node to be added by classes derived from Simulation
        virtual void addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory);

        virtual void resolveMigration();

        suids::distributed_generator<IRelationship> relationshipSuidGenerator;

        bool report_relationship_start;
        bool report_relationship_end;
        bool report_relationship_consummated;
        bool report_transmission;

    private:
#if USE_BOOST_SERIALIZATION
        template<class Archive>
        friend void serialize(Archive & ar, SimulationSTI &sim, const unsigned int  file_version );
#endif
        TypedPrivateMigrationQueueStorage<IndividualHumanSTI> typed_migration_queue_storage;
    };
}

DECLARE_VIRTUAL_BASE_OF(Kernel::Simulation, Kernel::SimulationSTI)
