/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#pragma warning(disable:4996)

#ifdef WIN32
#include "windows.h"
#endif
#include "Debug.h"
#include "Environment.h"
#include "Configuration.h"
#include "EventTrigger.h"

#include "Simulation.h"
#include "SimulationFactory.h"
#include "NodeProperties.h"
#include "Properties.h"

#ifndef _DLLS_
#ifndef DISABLE_MALARIA
#include "SimulationMalaria.h"
#endif
#ifndef DISABLE_VECTOR
#include "SimulationVector.h"
#endif
#ifdef ENABLE_ENVIRONMENTAL
#include "SimulationEnvironmental.h"
#endif
#ifdef ENABLE_POLIO
#include "SimulationPolio.h"
#endif
#ifdef ENABLE_TYPHOID
#include "SimulationTyphoid.h"
#endif

#ifndef DISABLE_AIRBORNE
#include "SimulationAirborne.h"
#endif
#ifdef ENABLE_TBHIV
#include "SimulationTBHIV.h"
#endif
#ifndef DISABLE_STI
#include "SimulationSTI.h"
#endif
#ifndef DISABLE_HIV
#include "SimulationHIV.h"
#endif
#ifdef ENABLE_DENGUE
#include "SimulationDengue.h"
#endif
#endif

#ifdef ENABLE_PYTHON_FEVER
#include "SimulationPy.h"
#endif

#include "SerializedPopulation.h"

#include <chrono>
#include "FileSystem.h"
#include "EventTrigger.h"

SETUP_LOGGING( "SimulationFactory" )

namespace Kernel
{
    ISimulation * SimulationFactory::CreateSimulation()
    {
        EventTriggerFactory::GetInstance()->Configure(EnvPtr->Config);
        NPFactory::CreateFactory();
        IPFactory::CreateFactory();

        ISimulation* newsim = nullptr;

        if ( CONFIG_PARAMETER_EXISTS( EnvPtr->Config, "Serialized_Population_Filenames" ) )
        {
            std::string path(".");
            if ( CONFIG_PARAMETER_EXISTS( EnvPtr->Config, "Serialized_Population_Path" ) )
            {
                path = GET_CONFIG_STRING(EnvPtr->Config, "Serialized_Population_Path");
            }
            std::vector<string> filenames = GET_CONFIG_VECTOR_STRING( EnvPtr->Config, "Serialized_Population_Filenames" );
            if ( filenames.size() != EnvPtr->MPI.NumTasks )
            {
                throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "MPI.NumTasks", float(EnvPtr->MPI.NumTasks), "filenames.size()", float(filenames.size()), "Number of serialized population filenames doesn't match number of MPI tasks.");
            }
            std::string population_filename = FileSystem::Concat(path, filenames[EnvPtr->MPI.Rank]);
            auto t_start = std::chrono::high_resolution_clock::now();
            newsim = SerializedState::LoadSerializedSimulation( population_filename.c_str() );
            auto t_finish = std::chrono::high_resolution_clock::now();
            newsim->Initialize( EnvPtr->Config );
            double elapsed = uint64_t((t_finish - t_start).count()) * 1000 * double(std::chrono::high_resolution_clock::period::num) / double(std::chrono::high_resolution_clock::period::den);
            LOG_INFO_F( "Loaded serialized population from '%s' in %f ms\n.", population_filename.c_str(), elapsed );
            return newsim;
        }

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
#ifdef ENABLE_ENVIRONMENTAL
            else if (sSimType == "ENVIRONMENTAL_SIM")
                sim_type = SimType::ENVIRONMENTAL_SIM;
#endif
#ifdef ENABLE_POLIO
            else if (sSimType == "POLIO_SIM")
                sim_type = SimType::POLIO_SIM;
#endif
#ifdef ENABLE_ENVIRONMENTAL
            else if (sSimType == "ENVIRONMENTAL_SIM")
                sim_type = SimType::ENVIRONMENTAL_SIM;
#endif
#ifdef ENABLE_TYPHOID
            else if (sSimType == "TYPHOID_SIM")
                sim_type = SimType::TYPHOID_SIM;
#endif
#ifndef DISABLE_AIRBORNE
            else if (sSimType == "AIRBORNE_SIM")
                sim_type = SimType::AIRBORNE_SIM;
#endif
#ifdef ENABLE_TBHIV
            else if (sSimType == "TBHIV_SIM")
                sim_type = SimType::TBHIV_SIM;
#endif // TBHIV
#ifndef DISABLE_STI
            else if (sSimType == "STI_SIM")
                sim_type = SimType::STI_SIM;
#endif
#ifndef DISABLE_HIV
            else if (sSimType == "HIV_SIM")
                sim_type = SimType::HIV_SIM;
#endif // HIV
#ifdef ENABLE_DENGUE
            else if (sSimType == "DENGUE_SIM")
                sim_type = SimType::DENGUE_SIM;
#endif
#ifdef ENABLE_PYTHON_FEVER
            else if (sSimType == "PY_SIM")
                sim_type = SimType::PY_SIM;
#endif
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
#if defined(ENABLE_ENVIRONMENTAL)
                case SimType::ENVIRONMENTAL_SIM:
                    newsim = SimulationEnvironmental::CreateSimulation(EnvPtr->Config);
                break;
#endif
#if defined( ENABLE_POLIO)
                case SimType::POLIO_SIM:
                    newsim = SimulationPolio::CreateSimulation(EnvPtr->Config);
                break;
#endif        
#if defined( ENABLE_TYPHOID)
                case SimType::TYPHOID_SIM:
                    newsim = SimulationTyphoid::CreateSimulation(EnvPtr->Config);
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

#ifndef DISABLE_AIRBORNE
                case SimType::AIRBORNE_SIM:
                    throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "AIRBORNE_SIM currently disabled. Consider using GENERIC_SIM or TBHIV_SIM." );
                    newsim = SimulationAirborne::CreateSimulation(EnvPtr->Config);
                break;
#endif

#ifdef ENABLE_TBHIV
                case SimType::TBHIV_SIM:
                    newsim = SimulationTBHIV::CreateSimulation(EnvPtr->Config);
                break;
#endif // TBHIV

#ifndef DISABLE_STI
                case SimType::STI_SIM:
                    newsim = SimulationSTI::CreateSimulation(EnvPtr->Config);
                break;
#endif
#ifndef DISABLE_HIV 
                case SimType::HIV_SIM:
                    newsim = SimulationHIV::CreateSimulation(EnvPtr->Config);
                break;
#endif // HIV
#ifdef ENABLE_DENGUE
                case SimType::DENGUE_SIM:
                    newsim = SimulationDengue::CreateSimulation(EnvPtr->Config);
                break;
#endif 
#ifdef ENABLE_PYTHON_FEVER 
                case SimType::PY_SIM:
                    newsim = SimulationPy::CreateSimulation(EnvPtr->Config);
                break;
#endif
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
            throw e;
        }

        return newsim;
    }
}
