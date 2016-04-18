/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#include "stdafx.h"

#include "ReportNodeDemographics.h"
#include "DllInterfaceHelper.h"
#include "FactorySupport.h"

#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "ReportUtilities.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Module name for logging, CustomReport.json, and DLL GetType()
static const char * _module = "ReportNodeDemographics";// <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "*", nullptr };// <<< Types of simulation the report is to be used with

report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportNodeDemographics()); // <<< Report to create
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
// --- ReportNodeDemographics Methods
// ----------------------------------------

    ReportNodeDemographics::ReportNodeDemographics()
        : BaseTextReport( "ReportNodeDemographics.csv" )
        , m_AgeYears()
        , m_Data()
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportNodeDemographics::~ReportNodeDemographics()
    {
    }

    bool ReportNodeDemographics::Configure( const Configuration * inputJson )
    {
        if( inputJson->Exist("Age_Bins") )
        {
            initConfigTypeMap("Age_Bins", &m_AgeYears, "Age Bins (in years) to aggregate within and report");
        }
        else
        {
            m_AgeYears.push_back( 40.0 );
            m_AgeYears.push_back( 80.0 );
            m_AgeYears.push_back( 125.0 );
        }
        bool ret = JsonConfigurable::Configure( inputJson );
        
        if( ret )
        {
            // initialize the counters so that they can be indexed by gender and age
            for( int g = 0 ; g < 2 ; g++ )
            {
                m_Data.push_back( std::vector<NodeData>() );
                for( int a = 0 ; a < m_AgeYears.size() ; a++ )
                {
                    NodeData nd;
                    m_Data[ g ].push_back( nd );
                }
            }
        }
        return ret;
    }

    std::string ReportNodeDemographics::GetHeader() const
    {
        std::stringstream header ;
        header << "Time"             << ", "
               << "NodeID"           << ", "
               << "Gender"           << ", "
               << "AgeYears"         << ", "
               << "NumIndividuals"   << ", "
               << "NumInfected"
               ;

        return header.str();
    }

    bool ReportNodeDemographics::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return true ;
    }

    void ReportNodeDemographics::LogIndividualData( IIndividualHuman* individual ) 
    {
        int gender_index  = (int)(individual->GetGender());
        int age_bin_index = ReportUtilities::GetAgeBin( individual->GetAge(), m_AgeYears );

        m_Data[ gender_index ][ age_bin_index ].num_people += 1;

        if( individual->IsInfected() )
        {
            m_Data[ gender_index ][ age_bin_index ].num_infected += 1;
        }
    }

    void ReportNodeDemographics::LogNodeData( INodeContext* pNC )
    {
        float time = pNC->GetTime().time;
        auto node_id = pNC->GetExternalID();

        for( int g = 0 ; g < 2 ; g++ )
        {
            char* gender = "M" ;
            if( g == 1 )
            {
                gender = "F";
            }
            for( int a = 0 ; a < m_AgeYears.size() ; a++ )
            {
                GetOutputStream() << time
                           << "," << node_id
                           << "," << gender 
                           << "," << m_AgeYears[ a ] 
                           << "," << m_Data[ g ][ a ].num_people 
                           << "," << m_Data[ g ][ a ].num_infected 
                           << std::endl;
            }
        }

        // Reset the counters for the next node
        for( int g = 0 ; g < 2 ; g++ )
        {
            for( int a = 0 ; a < m_AgeYears.size() ; a++ )
            {
                NodeData nd ;
                m_Data[ g ][ a ] = nd;
            }
        }
    }
}