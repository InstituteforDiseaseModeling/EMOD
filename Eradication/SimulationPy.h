/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Simulation.h"
#include "NodePy.h"
#include "InfectionPy.h"
#include "SusceptibilityPy.h"
#include "SimulationFactory.h"

namespace Kernel
{
    class NodePy;

    class SimulationPy : public Simulation
    {
    public:
        SimulationPy();
        static SimulationPy *CreateSimulation();
        static SimulationPy *CreateSimulation(const ::Configuration *config);
        virtual ~SimulationPy(void) { }
        virtual void Reports_CreateBuiltIn();

    protected:
        static bool ValidateConfiguration(const ::Configuration *config);

        virtual void Initialize() override;
        virtual void Initialize(const ::Configuration *config) override;

        // Allows correct type of community to be added by derived class Simulations
        virtual void addNewNodeFromDemographics( suids::suid node_suid,
                                                 NodeDemographicsFactory *nodedemographics_factory,
                                                 ClimateFactory *climate_factory,
                                                 bool white_list_enabled ) override;

        virtual void InitializeFlags(const ::Configuration *config);
        virtual void resolveMigration();

    private:

        friend class Kernel::SimulationFactory; // allow them to create us
    };
}
