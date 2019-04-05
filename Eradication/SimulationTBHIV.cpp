/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "Exceptions.h"
#include "SimulationTBHIV.h"
#include "NodeTBHIV.h"

// TBHIV
#include "ReportTBHIV.h"

// TB
#include "PropertyReportTB.h"
#include "SpatialReportTB.h"
#include "BinnedReportTB.h"


SETUP_LOGGING( "SimulationTBHIV" )

namespace Kernel
{
    SimulationTBHIV::~SimulationTBHIV(void) { }
    SimulationTBHIV::SimulationTBHIV()
    {
        //TBHIV Reports
        reportClassCreator = ReportTBHIV::CreateReport;
        
        //TB Reports
        //reportClassCreator = ReportTB::CreateReport;
        binnedReportClassCreator = BinnedReportTB::CreateReport;
        spatialReportClassCreator = SpatialReportTB::CreateReport;
        propertiesReportClassCreator = PropertyReportTB::CreateReport;
    }

    SimulationTBHIV *SimulationTBHIV::CreateSimulation()
    {
        SimulationTBHIV *newsimulation = _new_ SimulationTBHIV();

        return newsimulation;
    }

    SimulationTBHIV *SimulationTBHIV::CreateSimulation(const ::Configuration *config)
    {
        SimulationTBHIV *newsimulation = nullptr;
        newsimulation = _new_ SimulationTBHIV();
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

    bool SimulationTBHIV::ValidateConfiguration(const ::Configuration *config)
    {
        if (!SimulationAirborne::ValidateConfiguration(config))
            return false;

        // TODO: are there any more checks on configuration parameters we want to do here?

        return true;
    }

    void SimulationTBHIV::Initialize( const ::Configuration *config )
    {
        SimulationAirborne::Initialize( config );
        IndividualHumanCoInfection::InitializeStaticsCoInfection( config );
    }

    void SimulationTBHIV::addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                      suids::suid node_suid,
                                                      NodeDemographicsFactory *nodedemographics_factory,
                                                      ClimateFactory *climate_factory,
                                                      bool white_list_enabled )
    {
        NodeTBHIV *node = NodeTBHIV::CreateNode(this, externalNodeId, node_suid);
        addNode_internal( node, nodedemographics_factory, climate_factory, white_list_enabled );
    }


    REGISTER_SERIALIZABLE(SimulationTBHIV);

    void SimulationTBHIV::serialize(IArchive& ar, SimulationTBHIV* obj)
    {
        SimulationAirborne::serialize(ar, obj);
        // Nothing to do here
    }

}

