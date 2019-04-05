/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#if defined(ENABLE_ENVIRONMENTAL)

#include "SimulationEnvironmental.h"
#include "NodeEnvironmental.h"
#include "InfectionEnvironmental.h"
#include "SusceptibilityEnvironmental.h"
#include "ReportEnvironmental.h"
#include "PropertyReportEnvironmental.h"
#include "SimulationConfig.h"
#include "ProgVersion.h"

using namespace Kernel;

#pragma warning(disable : 4996)

SETUP_LOGGING( "SimulationEnvironmental" )

namespace Kernel
{
    SimulationEnvironmental::SimulationEnvironmental() : Simulation()
    {
        reportClassCreator = ReportEnvironmental::CreateReport;
        propertiesReportClassCreator = PropertyReportEnvironmental::CreateReport;
    }

    SimulationEnvironmental::~SimulationEnvironmental(void)
    {
    }

    void SimulationEnvironmental::Initialize()
    {
        Simulation::Initialize();
    }

    void SimulationEnvironmental::Initialize(const ::Configuration *config)
    {
        Simulation::Initialize(config);
    }

    SimulationEnvironmental *SimulationEnvironmental::CreateSimulation()
    {
        SimulationEnvironmental *newsimulation = _new_ SimulationEnvironmental();
        newsimulation->Initialize();

        return newsimulation;
    }

    SimulationEnvironmental *SimulationEnvironmental::CreateSimulation(const ::Configuration *config)
    {
        SimulationEnvironmental *newsimulation = nullptr;

        newsimulation = _new_ SimulationEnvironmental();
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

    bool SimulationEnvironmental::ValidateConfiguration(const ::Configuration *config)
    {
        if (!Kernel::Simulation::ValidateConfiguration(config))
            return false;

        // TODO: any disease-specific validation goes here.
        // Warning: static climate parameters are not configured until after this function is called

        return true;
    }
    void SimulationEnvironmental::addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                              suids::suid node_suid,
                                                              NodeDemographicsFactory *nodedemographics_factory,
                                                              ClimateFactory *climate_factory,
                                                              bool white_list_enabled )
    {
        NodeEnvironmental *node = NodeEnvironmental::CreateNode( this, externalNodeId, node_suid );

        addNode_internal( node, nodedemographics_factory, climate_factory, white_list_enabled );
    }

    void SimulationEnvironmental::InitializeFlags( const ::Configuration *config )
    {
    }
}

#endif // ENABLE_POLIO
