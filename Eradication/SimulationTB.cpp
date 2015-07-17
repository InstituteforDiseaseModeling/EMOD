/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "Exceptions.h"
#include "SimulationTB.h"
#include "PropertyReportTB.h"

#include "InfectionTB.h"
#include "NodeTB.h"
#include "SusceptibilityTB.h"
#include "ReportTB.h"
#include "SpatialReportTB.h"
#include "BinnedReportTB.h"

static const char* _module = "SimulationTB";

namespace Kernel
{
    SimulationTB::~SimulationTB(void) { }
    SimulationTB::SimulationTB()
    {
        reportClassCreator = ReportTB::CreateReport;
        binnedReportClassCreator = BinnedReportTB::CreateReport;
        spatialReportClassCreator = SpatialReportTB::CreateReport;
        propertiesReportClassCreator = PropertyReportTB::CreateReport;
    }

    SimulationTB *SimulationTB::CreateSimulation()
    {
        SimulationTB *newsimulation = _new_ SimulationTB();

        return newsimulation;
    }

    SimulationTB *SimulationTB::CreateSimulation(const ::Configuration *config)
    {
        SimulationTB *newsimulation = NULL;
        newsimulation = _new_ SimulationTB();
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

    bool SimulationTB::ValidateConfiguration(const ::Configuration *config)
    {
        if (!SimulationAirborne::ValidateConfiguration(config))
            return false;

        // TODO: are there any more checks on configuration parameters we want to do here?

        return true;
    }

    void SimulationTB::Initialize( const ::Configuration *config )
    {
        SimulationAirborne::Initialize( config );
        IndividualHumanTB fakeHuman;
        LOG_INFO( "Calling Configure on fakeHuman\n" );
        fakeHuman.Configure( config );
    }

    void SimulationTB::addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory)
    {
        NodeTB *node = NodeTB::CreateNode(this, node_suid);
        addNode_internal(node, nodedemographics_factory, climate_factory);
    }

    void SimulationTB::resolveMigration()
    {
        resolveMigrationInternal(typed_migration_queue_storage, migratingIndividualQueues);
    }
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::SimulationTB)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, SimulationTB &sim, const unsigned int  file_version )
    {
        // Register derived types
        ar.template register_type<NodeTB>();
        ar.template register_type<NodeTBFlags>();
        ar.template register_type<SusceptibilityTBFlags>();
        ar.template register_type<InfectionTBFlags>();

        // Serialize base class
        ar & boost::serialization::base_object<SimulationAirborne>(sim);
    }
}
#endif

#endif // ENABLE_TB
