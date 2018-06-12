/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReportEventCounter.h"
#include "Environment.h"
#include "DllInterfaceHelper.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportEventCounter" ) // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "*", nullptr };// <<< Types of simulation the report is to be used with

report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportEventCounter()); // <<< Report to create
};

DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ------------------------------
// --- DLL Interface Methods
// ---
// --- The DTK will use these methods to establish communication with the DLL.
// ------------------------------

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

DTK_DLLEXPORT char* __cdecl
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
}

DTK_DLLEXPORT void __cdecl
GetSupportedSimTypes(char* simTypes[])
{
    DLL_HELPER.GetSupportedSimTypes( simTypes );
}

DTK_DLLEXPORT const char * __cdecl
GetType()
{
    return DLL_HELPER.GetType();
}

DTK_DLLEXPORT void __cdecl
GetReportInstantiator( Kernel::report_instantiator_function_t* pif )
{
    DLL_HELPER.GetReportInstantiator( pif );
}

#ifdef __cplusplus
}
#endif

// ----------------------------------------
// --- ReportEventCounter Methods
// ----------------------------------------

    ReportEventCounter::ReportEventCounter()
        : BaseEventReport( "ReportEventCounter" )
        , channelDataMap()
        , unitsMap()
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportEventCounter::~ReportEventCounter()
    {
    }

    bool ReportEventCounter::Configure( const Configuration * inputJson )
    {
        bool ret = BaseEventReport::Configure( inputJson );

        if( ret )
        {
            const std::vector< EventTrigger >& trigger_list = GetEventTriggerList();
            for( auto trigger : trigger_list )
            {
                unitsMap[ trigger.ToString() ] = "" ;
                channelDataMap.AddChannel( trigger.ToString() );
            }
        }
        return ret;
    }

    void ReportEventCounter::BeginTimestep()
    {
        channelDataMap.IncreaseChannelLength( 1 );
    }

    void ReportEventCounter::Reduce()
    {
        channelDataMap.Reduce();
        BaseEventReport::Reduce();
    }

    void ReportEventCounter::Finalize()
    {
        std::string output_fn = GetBaseOutputFilename() + ".json" ;
        channelDataMap.WriteOutput( output_fn, unitsMap );
        BaseEventReport::Finalize();
    }

    bool ReportEventCounter::notifyOnEvent( IIndividualHumanEventContext *context, 
                                            const EventTrigger& trigger )
    {
        if( HaveUnregisteredAllEvents() )
        {
            return false ;
        }

        channelDataMap.Accumulate( trigger.ToString(), 1.0 );

        return true ;
    }

}