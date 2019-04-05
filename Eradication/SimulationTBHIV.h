/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "SimulationAirborne.h"
#include "NodeTBHIV.h" // for serialization forward reg ONLY
#include "IndividualCoInfection.h"

namespace Kernel
{
    class SimulationTBHIV : public SimulationAirborne
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
       
    public:
        static   SimulationTBHIV *CreateSimulation();
        static   SimulationTBHIV *CreateSimulation(const ::Configuration *config);
        virtual void Initialize( const ::Configuration *config ) override;
        virtual ~SimulationTBHIV(void);

    protected:

        SimulationTBHIV();

        static bool ValidateConfiguration(const ::Configuration *config);

        // Allows correct type of Node to be added by classes derived from Simulation
        virtual void addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                 suids::suid node_suid,
                                                 NodeDemographicsFactory *nodedemographics_factory,
                                                 ClimateFactory *climate_factory,
                                                 bool white_list_enabled ) override;
        
        DECLARE_SERIALIZABLE(SimulationTBHIV);
    };

   
}
