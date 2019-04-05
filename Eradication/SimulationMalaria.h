/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BoostLibWrapper.h"

#include "MalariaContexts.h"
#include "SimulationVector.h"

namespace Kernel
{
    class SimulationMalaria : public SimulationVector
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        SimulationMalaria();
        static SimulationMalaria *CreateSimulation();
        static SimulationMalaria *CreateSimulation(const ::Configuration *config);
        virtual ~SimulationMalaria();

        // Allows correct type of community to be added by derived class Simulations
        virtual void addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                 suids::suid node_suid,
                                                 NodeDemographicsFactory *nodedemographics_factory,
                                                 ClimateFactory *climate_factory,
                                                 bool white_list_enabled ) override;

    protected:
        static bool ValidateConfiguration(const ::Configuration *config);

        virtual void InitializeFlags(const ::Configuration *config);  // override in derived classes to instantiate correct flag classes

        DECLARE_SERIALIZABLE(SimulationMalaria);

    private:
        virtual void Initialize(const ::Configuration *config) override;

        virtual ISimulationContext *GetContextPointer() override;
    };
}
