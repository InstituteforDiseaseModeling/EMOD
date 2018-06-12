/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <iostream>
#include <queue>
#include <iomanip> //setw(), setfill()
#include <algorithm>

#include "BoostLibWrapper.h"
#include "FileSystem.h"
#include "Debug.h"
#include "Log.h"
#include "suids.hpp"
#include "SimulationConfig.h"
#include "SimulationFactory.h"
#include "Simulation.h"
#include "IdmMpi.h"

#ifndef _DLLS_
#include "SimulationMalaria.h"
#ifdef ENABLE_POLIO
#include "SimulationPolio.h"
#endif

#ifdef ENABLE_TBHIV
#include "SimulationTBHIV.h"
#endif // TBHIV
#endif // _DLLS_
#include "ControllerFactory.h"

#include "SerializedPopulation.h"

#pragma warning(disable : 4244)

using namespace Kernel;

SETUP_LOGGING( "Controller" )


// more static polymorphism that is temporary until we determine why
// boost refuses to register the sim type with the archive

template<class ControllerExecuteFunctorT>
bool call_templated_functor_with_sim_type_hack(ControllerExecuteFunctorT &cef)
{
#ifdef _DLLS_
    return cef.template call<Simulation>();
#else
    //SimType::Enum sim_type = GET_CONFIGURABLE(SimulationConfig)->sim_type;
    std::string sSimType = GET_CONFIG_STRING(EnvPtr->Config, "Simulation_Type");      
    SimType::Enum sim_type;
    if (sSimType == "GENERIC_SIM")
        sim_type = SimType::GENERIC_SIM;
    else if (sSimType == "MALARIA_SIM")
        sim_type = SimType::MALARIA_SIM;
    else if (sSimType == "VECTOR_SIM")
        sim_type = SimType::VECTOR_SIM;
#ifdef ENABLE_POLIO
    else if (sSimType == "ENVIRONMENTAL_SIM")
        sim_type = SimType::ENVIRONMENTAL_SIM;
    else if (sSimType == "POLIO_SIM")
        sim_type = SimType::POLIO_SIM;
#endif
#ifdef ENABLE_TBHIV
    else if (sSimType == "AIRBORNE_SIM")
        sim_type = SimType::AIRBORNE_SIM;
    else if (sSimType == "TBHIV_SIM")
        sim_type = SimType::TBHIV_SIM;
#endif // TBHIV
    else
    {
        std::string note = "The Simulation_Type (='"+sSimType+"') is unknown.  Please select a valid type." ;
        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, note.c_str() );
    }

    switch (sim_type)
    {
        case SimType::GENERIC_SIM:       return cef.template call<Simulation>();
        case SimType::VECTOR_SIM:        return cef.template call<SimulationVector>();
        case SimType::MALARIA_SIM:       return cef.template call<SimulationMalaria>();
#ifdef ENABLE_POLIO
        case SimType::ENVIRONMENTAL_SIM: return cef.template call<SimulationEnvironmental>();
        case SimType::POLIO_SIM:         return cef.template call<SimulationPolio>();
#endif
#ifdef ENABLE_TBHIV
        case SimType::AIRBORNE_SIM:      return cef.template call<SimulationAirborne>();
        case SimType::TBHIV_SIM:         return cef.template call<SimulationTBHIV>();
#endif // TBHIV
    default: 
        // ERROR: ("call_templated_functor_with_sim_type_hack(): Error, Sim_Type %d is not implemented.\n", sim_type);
        throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "sim_type", sim_type, SimType::pairs::lookup_key( sim_type ) );
    }
#endif
}

#include "Instrumentation.h"
#include "StatusReporter.h"

#include <functional>

void StepSimulation(ISimulation* sim, float dt);

typedef enum {
    paused,
    stepping,
    stepping_and_reloading,
    playing
} tPlayback;

tPlayback playback = playing;

// Basic simulation main loop with reporting
template <class SimulationT> 
void RunSimulation(SimulationT &sim, int steps, float dt)
{
    LOG_DEBUG( "RunSimulation\n" );

    std::queue< int32_t > serialization_time_steps;
    if ( CONFIG_PARAMETER_EXISTS(EnvPtr->Config, "Serialization_Time_Steps") )
    {
        vector< int32_t > specified_time_steps = GET_CONFIG_VECTOR_INT( EnvPtr->Config, "Serialization_Time_Steps" );
        // Sort specified_time_steps in case the user entered the values out of order.
        std::sort( specified_time_steps.begin(), specified_time_steps.end() );
        for (int32_t time_step : specified_time_steps)
        {
            serialization_time_steps.push( time_step );
        }
    }
    else
    {
        serialization_time_steps.push( -1 );
    }

    for (int t = 0; t < steps; t++)
    {
        // Specifying serialization time step 0 means "please serialize the initial state."
        if ( (t == 0) && (serialization_time_steps.size() > 0) && (serialization_time_steps.front() == t) )
        {
            SerializedState::SaveSerializedSimulation(dynamic_cast<Simulation*>(&sim), t, true);
            serialization_time_steps.pop();
        }

        StepSimulation(&sim, dt);

        // Compare against t + 1 so, e.g., a simulation of duration 90 can specify serialization time step 90 to save the final state.
        int step = t + 1;
        if ( (serialization_time_steps.size() > 0) && (serialization_time_steps.front() == step) )
        {
            // Using step, (t+1), here too.
            SerializedState::SaveSerializedSimulation(dynamic_cast<Simulation*>(&sim), step, true);
            serialization_time_steps.pop();
        }

        if (EnvPtr->MPI.Rank == 0)
        {
            EnvPtr->getStatusReporter()->ReportProgress(t+1, steps);
        }
    }
}

// note: this version passes a branch_duration that counts only timesteps taken within itself
// if branches have more complicated logic, we may want to put some of that outside.
template <class SimulationT>
void RunSimulation(SimulationT &sim, std::function<bool(SimulationT &, float)> termination_predicate) // TODO: add support for 'dt' to this version
{
    LOG_DEBUG( "RunSimulation\n" );

    float branch_begin = sim.GetSimulationTime();
    float branch_time  = 0;
    float dt           = float(GET_CONFIGURABLE(SimulationConfig)->Sim_Tstep);

    while(!termination_predicate(sim, branch_time))
    {
        StepSimulation(&sim, dt);
        branch_time = sim.GetSimulationTime() - branch_begin;
    }
}

void StepSimulation(ISimulation* sim, float dt)
{
    sim->Update(dt);

    EnvPtr->Log->Flush();
}

// ******** WARNING *********
// ENTERING MASSIVE HACK ZONE 

// this class mimics the interface of scoped_ptr but actually does nothing. 
// its a quick hack to try ducking very expensive and unnecessary object cleanup associated 
// runs that only instantiate one simulation and then exit...while keeping the code formally 
// similar to a 'proper' implementation
// this would all go away with more thoughtful use of allocators, and perhaps custom allocators
// - in particular, scoping the allocation blocks for boost pool allocators around the simulation lifetimes would help

template<class T>
class MassivelyHackedLeakyPointer
{
public:
    MassivelyHackedLeakyPointer(T* _ptr) : px(_ptr) {}

    void reset(T * p = nullptr) // never throws
    {
        BOOST_ASSERT( p == nullptr || p != px ); // catch self-reset errors
        px = p;
    }

    T & operator*() const // never throws
    {
        BOOST_ASSERT( px != nullptr );
        return *px;
    }

    T * operator->() const // never throws
    {
        BOOST_ASSERT( px != nullptr );
        return px;
    }

    T * get() const // never throws
    {
        return px;
    }

    bool operator==( MassivelyHackedLeakyPointer<T> const& o ) const { return o->px == px; }
    bool operator==( T* const& o ) const { return o == px; }

protected:
    T * px;
};

template <class SimulationT>
bool DefaultController::execute_internal()
{
    using namespace Kernel;
    list<string> serialization_test_state_filenames;
    //typedef Simulation SimulationT ;

    LOG_INFO("DefaultController::Execute<>()...\n");

    // NB: BIG INTENTIONAL HACK
    // the exact nature of pool allocators substantially helps communication performance BUT unwinding them all at the end can double the simulation runtime for a real production scenario.
    // for processes that dont need to have more than one simulation in memory, its faster to just leak the whole object

#ifdef _DEBUG
    boost::scoped_ptr<SimulationT> sim(dynamic_cast<SimulationT*>(SimulationFactory::CreateSimulation())); // 30+ minutes to unwind a ~2gb simulation state if we do this. unacceptable for real work!
#else
#ifdef _DLLS_
    ISimulation * sim = SimulationFactory::CreateSimulation(); 
    release_assert(sim);
#else
    MassivelyHackedLeakyPointer<SimulationT> sim(dynamic_cast<SimulationT*>(SimulationFactory::CreateSimulation())); 

    if (nullptr == sim.get())
    {
        throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "sim.get() returned NULL after call to CreateSimulation." );
    }
#endif
#endif

    if (EnvPtr->MPI.Rank==0) { ostringstream oss; oss << "Beginning Simulation...";  EnvPtr->getStatusReporter()->ReportStatus(oss.str()); }

    // populate it
    LOG_INFO("DefaultController populate simulation...\n");
    if(sim->Populate())
    {
        if( !JsonConfigurable::missing_parameters_set.empty() )
        {
            std::stringstream errMsg;
            errMsg << "The following necessary parameters were not specified" << std::endl;
            for (auto& key : JsonConfigurable::missing_parameters_set)
            {
                errMsg << "\t \"" << key.c_str() << "\"" << std::endl;
            }
            //LOG_ERR( errMsg.str().c_str() );
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
        }
        // now try to run it
        // divide the simulation into stages according to requesting number of serialization test cycles
        float dt = GET_CONFIGURABLE(SimulationConfig)->Sim_Tstep;
        int simulation_steps = int(GET_CONFIGURABLE(SimulationConfig)->Sim_Duration)/dt;

#ifndef _DLLS_
        int remaining_steps = simulation_steps;

        for (int k= 0; remaining_steps > 0; k++)
        {
            int cycle_steps = min(remaining_steps, max(1, simulation_steps));
            if (cycle_steps > 0)
                RunSimulation(*sim, cycle_steps, dt);

            remaining_steps -= cycle_steps;
        }
#else
        LOG_INFO( "Execute<> Calling RunSimulation.\n" );
        RunSimulation(*sim, simulation_steps, dt);
#endif
        sim->WriteReportsData();

        if (EnvPtr->MPI.Rank==0)
        {
            LogTimeInfo lti;
            EnvPtr->Log->GetLogInfo(lti);

            ostringstream oss;
            oss << "Done - " << lti.hours << ":" << setw(2) << setfill('0') << lti.mins << ":" << setw(2) << setfill('0') << lti.secs;
            EnvPtr->getStatusReporter()->ReportStatus(oss.str());
        }

        // cleanup serialization test state files
        for (auto& filename : serialization_test_state_filenames)
        {
            if( FileSystem::FileExists( filename ) )
                FileSystem::RemoveFile( filename );
        }

        LOG_INFO_F( "Exiting %s\n", __FUNCTION__ );

        return true;
    }
    
    return false;
}


bool DefaultController::execute_internal()
{

    using namespace Kernel;
    list<string> serialization_test_state_filenames;

    LOG_INFO("DefaultController::execute_internal()...\n");

#ifdef _DLLS_
    ISimulation * sim = SimulationFactory::CreateSimulation(); 
    release_assert(sim);
#else

    boost::scoped_ptr<ISimulation> sim((SimulationFactory::CreateSimulation()));

    if (nullptr == sim.get())
    {
        throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "sim.get() returned NULL after call to CreateSimulation.\n" );
    }

#endif // End of _DLLS_

    if (EnvPtr->MPI.Rank==0) { ostringstream oss; oss << "Beginning Simulation...";  EnvPtr->getStatusReporter()->ReportStatus(oss.str()); }

    // populate it
    LOG_INFO("DefaultController::execute_internal() populate simulation...\n");
    // Confusing variable name (JC::useDefaults); we want to collect all defaults for reporting. It's up to us as the calling function
    JsonConfigurable::_track_missing = true;
    if(sim->Populate())
    {
        // Need to reset back to false; will be set as appropriate by campaign related code after this based on
        // "Use_Defaults" in campaign.json.
        JsonConfigurable::_useDefaults = false;
        if( !JsonConfigurable::missing_parameters_set.empty() )
        {
            std::stringstream errMsg;
            errMsg << "The following necessary parameters were not specified" << std::endl;
            for (auto& key : JsonConfigurable::missing_parameters_set)
            {
                errMsg << "\t \"" << key.c_str() << "\"" << std::endl;
            }
            //LOG_ERR( errMsg.str().c_str() );
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
        }
        // now try to run it
        // divide the simulation into stages according to requesting number of serialization test cycles
        float dt = GET_CONFIGURABLE(SimulationConfig)->Sim_Tstep;
        int simulation_steps = int(GET_CONFIGURABLE(SimulationConfig)->Sim_Duration)/dt;

#ifndef _DLLS_
        int remaining_steps = simulation_steps;

        for (int k= 0; remaining_steps > 0; k++)
        {
            int cycle_steps = min(remaining_steps, max(1, simulation_steps));
            if (cycle_steps > 0)
                RunSimulation(*sim, cycle_steps, dt);

            remaining_steps -= cycle_steps;
        }
#else // _DLLS_
        LOG_INFO( "Execute_internal(): Calling RunSimulation.\n" );
        RunSimulation(*sim, simulation_steps, dt);
#endif
        sim->WriteReportsData();

        if (EnvPtr->MPI.Rank==0)
        {
            LogTimeInfo lti;
            EnvPtr->Log->GetLogInfo(lti);

            ostringstream oss;
            oss << "Done - " << lti.hours << ":" << setw(2) << setfill('0') << lti.mins << ":" << setw(2) << setfill('0') << lti.secs;
            EnvPtr->getStatusReporter()->ReportStatus(oss.str());
        }

        // cleanup serialization test state files
        for (auto& filename : serialization_test_state_filenames)
        {
            if( FileSystem::FileExists( filename ) )
                FileSystem::RemoveFile( filename );
        }

        LOG_INFO_F( "Exiting %s\n", __FUNCTION__ );

        return true;
    }
    return false;
}

bool DefaultController::Execute()
{
    return execute_internal();
}
