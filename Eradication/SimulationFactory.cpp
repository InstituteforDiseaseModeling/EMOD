/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#ifdef WIN32
#include "windows.h"
#endif
#include "Debug.h"
#include "Environment.h"
#include "Configuration.h"
#include "SimulationConfig.h"

#include "Simulation.h"
#include "SimulationFactory.h"
#ifndef _DLLS_
#ifndef DISABLE_MALARIA
#include "SimulationMalaria.h"
#endif
#ifdef ENABLE_POLIO
#include "SimulationPolio.h"
#endif

#ifdef ENABLE_TB
#include "SimulationTB.h"
#endif

#ifdef ENABLE_TBHIV
#include "SimulationTBHIV.h"
#endif
#ifndef DISABLE_HIV
#include "SimulationSTI.h"
#include "SimulationHIV.h"
#endif
#endif

#include "DllLoader.h"

static const char * _module = "SimulationFactory";

namespace Kernel
{
    ISimulation * SimulationFactory::CreateSimulation()
    {

        ISimulation * newsim = NULL;
        std::string sSimType;

        try
        {
            sSimType = GET_CONFIG_STRING(EnvPtr->Config, "Simulation_Type");

            SimType::Enum sim_type;
            if (sSimType == "GENERIC_SIM")
                sim_type = SimType::GENERIC_SIM;
#ifndef DISABLE_VECTOR
            else if (sSimType == "VECTOR_SIM")
                sim_type = SimType::VECTOR_SIM;
#endif
#ifndef DISABLE_MALARIA
            else if (sSimType == "MALARIA_SIM")
                sim_type = SimType::MALARIA_SIM;
#endif
#ifdef ENABLE_POLIO
            else if (sSimType == "ENVIRONMENTAL_SIM")
                sim_type = SimType::ENVIRONMENTAL_SIM;
            else if (sSimType == "POLIO_SIM")
                sim_type = SimType::POLIO_SIM;
#endif
#ifdef ENABLE_TB
            else if (sSimType == "AIRBORNE_SIM")
                sim_type = SimType::AIRBORNE_SIM;
            else if (sSimType == "TB_SIM")
                sim_type = SimType::TB_SIM;
#ifdef ENABLE_TBHIV
            else if (sSimType == "TBHIV_SIM")
                sim_type = SimType::TBHIV_SIM;
#endif // TBHIV
#endif // TB
#ifndef DISABLE_HIV
            else if (sSimType == "STI_SIM")
                sim_type = SimType::STI_SIM;
            else if (sSimType == "HIV_SIM")
                sim_type = SimType::HIV_SIM;
#endif // HIV
            else
            {
                std::ostringstream msg;
                msg << "Simulation_Type " << sSimType << " not recognized." << std::endl;
                throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

#ifdef _DLLS_

            // Look through disease dll directory, do LoadLibrary on each .dll,
            // do GetProcAddress on GetMimeType() and CreateSimulation
            typedef ISimulation* (*createSim)(const Environment *);
            std::map< std::string, createSim > createSimFuncPtrMap;

            // Note map operator [] will automatically initialize the pointer to NULL if not found
            DllLoader dllLoader;         
            if (!dllLoader.LoadDiseaseDlls(createSimFuncPtrMap) || !createSimFuncPtrMap[sSimType])
            {
                std::ostringstream msg;
                msg << "Failed to load disease emodules for SimType: " << SimType::pairs::lookup_key(sim_type) << " from path: " << dllLoader.GetEModulePath(DISEASE_EMODULES).c_str() << std::endl;
                throw Kernel::DllLoadingException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str());
                return newsim;
            }
            newsim = createSimFuncPtrMap[sSimType](EnvPtr);
            release_assert(newsim);

#else // _DLLS_
            switch (sim_type)
            {
                case SimType::GENERIC_SIM:
                    newsim = Simulation::CreateSimulation(EnvPtr->Config);
                break;
#ifdef ENABLE_POLIO
                case SimType::ENVIRONMENTAL_SIM:
                    newsim = SimulationEnvironmental::CreateSimulation(EnvPtr->Config);
                break;

                case SimType::POLIO_SIM:
                    newsim = SimulationPolio::CreateSimulation(EnvPtr->Config);
                break;
#endif        
#ifndef DISABLE_VECTOR
                case SimType::VECTOR_SIM:
                    newsim = SimulationVector::CreateSimulation(EnvPtr->Config);
                break;
#endif
#ifndef DISABLE_MALARIA
                case SimType::MALARIA_SIM:
                    newsim = SimulationMalaria::CreateSimulation(EnvPtr->Config);
                break;
#endif
#ifdef ENABLE_TB
                case SimType::AIRBORNE_SIM:
                    newsim = SimulationAirborne::CreateSimulation(EnvPtr->Config);
                break;

                case SimType::TB_SIM:
                    newsim = SimulationTB::CreateSimulation(EnvPtr->Config);
                break;
#ifdef ENABLE_TBHIV
                case SimType::TBHIV_SIM:
                    newsim = SimulationTBHIV::CreateSimulation(EnvPtr->Config);
                break;
#endif // TBHIV
#endif // TB
#ifndef DISABLE_HIV 
                case SimType::STI_SIM:
                    newsim = SimulationSTI::CreateSimulation(EnvPtr->Config);
                break;

                case SimType::HIV_SIM:
                    newsim = SimulationHIV::CreateSimulation(EnvPtr->Config);
                break;
#endif // HIV

                default: 
                // Is it even possible to get here anymore?  Won't the SimulationConfig error out earlier parsing the parameter-string?
                //("SimulationFactory::CreateSimulation(): Error, Simulation_Type %d is not implemented.\n", sim_type);
                //throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "sim_type", sim_type, SimType::pairs::lookup_key( sim_typ )); // JB
                break;
            }
#endif
            release_assert(newsim);
        }
        catch ( GeneralConfigurationException& e ) {
            LOG_ERR(e.GetMsg());
            LOG_ERR("Caught GeneralConfigurationException trying to CreateSimulation(). Returning NULL for newsim.\n");
        }

        return newsim;
    }

}
