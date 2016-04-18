/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReportVectorStats.h"
#include "DllInterfaceHelper.h"
#include "FactorySupport.h"

#include "NodeEventContext.h"
#include "Individual.h"
#include "VectorContexts.h"
#include "VectorPopulation.h"
#include "VectorCohortIndividual.h"

//#define EVERY_VECTOR
#ifdef EVERY_VECTOR
#include "VectorPopulationIndividual.h"
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Module name for logging, CustomReport.json, and DLL GetType()
static const char * _module = "ReportVectorStats";// <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "VECTOR_SIM", "MALARIA_SIM", nullptr };// <<< Types of simulation the report is to be used with

report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new ReportVectorStats()); // <<< Report to create
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
// --- ReportVectorStats Methods
// ----------------------------------------

    ReportVectorStats::ReportVectorStats()
        : BaseTextReportEvents( "ReportVectorStats.csv" )
        , migration_count_local(0)
        , migration_count_regional(0)
        , species_list()
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();

        species_list.push_back( "arabiensis" );
        species_list.push_back( "funestus" );
        species_list.push_back( "gambiae" );
    }

    ReportVectorStats::~ReportVectorStats()
    {
    }

    bool ReportVectorStats::Configure( const Configuration * inputJson )
    {

        bool ret = BaseTextReportEvents::Configure( inputJson );


        // Manually push required events into the eventTriggerList
        //eventTriggerList.push_back( IndividualEventTriggerType::HIVNewlyDiagnosed );
        
        return ret;
    }

    void ReportVectorStats::UpdateEventRegistration(  float currentTime, 
                                                       float dt, 
                                                       std::vector<INodeEventContext*>& rNodeEventContextList )
    {
        BaseTextReportEvents::UpdateEventRegistration( currentTime, dt, rNodeEventContextList );
    }


    std::string ReportVectorStats::GetHeader() const
    {
        std::stringstream header ;
#ifdef EVERY_VECTOR
        header << "Time"                << ", "
               << "NodeID"              << ", "
               << "VecID"              ;
#else
        header << "Time"                << ", "
               << "NodeID"              << ", "
               << "Population"          << ", "
               << "VectorPopulation"    << ", "
               << "AdultCount"          << ", "
               << "InfectedCount"       << ", "
               << "InfectiousCount"     << ", "
               << "MaleCount"           << ", "
               << "NewEggsCount"        << ", "
               << "IndoorBitesCount"    << ", "
               << "OutdoorBitesCount"   << ", "
               << "MigrationFromCountLocal" << ", "
               << "MigrationFromCountRegional"
               ;
        for( auto sp : species_list )
        {
            header << ", " << sp << "_AvailableHabitat" ;
        }
        for( auto sp : species_list )
        {
            header << ", " << sp << "_EggCrowdingCorrection" ;
        }
#endif
        return header.str();
    }

    void ReportVectorStats::LogNodeData( Kernel::INodeContext* pNC )
    {
        auto time = pNC->GetTime().time ;
        auto nodeId = pNC->GetExternalID();
        auto pop = pNC->GetStatPop();
        
        INodeVector * pNodeVector = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pNodeVector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext");
        }

        int adult_count = 0 ;
        int infected_count = 0 ;
        int infectious_count = 0 ;
        int male_count = 0 ;
        int new_eggs_count = 0 ;
        int indoor_bites_count = 0 ;
        int outdoor_bites_count = 0 ;
        std::map<std::string,float> available_habitat_per_species ;
        std::map<std::string,float> egg_crowding_correction_per_species ;
        for( auto sp : species_list )
        {
            available_habitat_per_species.insert( std::make_pair( sp, 0.0f ) );
            egg_crowding_correction_per_species.insert( std::make_pair( sp, 0.0f ) );
        }
        std::list<VectorPopulation*> vector_pop_list = pNodeVector->GetVectorPopulations();
        for( auto vp : vector_pop_list )
        {
#ifdef EVERY_VECTOR
            VectorPopulationIndividual* vpi = dynamic_cast<VectorPopulationIndividual*>(vp);
            for( auto adult : vpi->AdultQueues )
            {
                VectorCohortIndividual* pvci = dynamic_cast<VectorCohortIndividual*>(adult);
                GetOutputStream() << time
                           << "," << nodeId
                           << "," << pvci->GetID();
                GetOutputStream() << endl;
            }
#else
            adult_count         += vp->getAdultCount();
            infected_count      += vp->getInfectedCount();
            infectious_count    += vp->getInfectiousCount();
            male_count          += vp->getMaleCount();
            new_eggs_count      += vp->getNewEggsCount();
            indoor_bites_count  += vp->GetHBRByPool( VectorPoolIdEnum::INDOOR_VECTOR_POOL );
            outdoor_bites_count += vp->GetHBRByPool( VectorPoolIdEnum::OUTDOOR_VECTOR_POOL );
            for( auto habitat : vp->GetHabitats() )
            {
                available_habitat_per_species[ vp->get_SpeciesID() ] += habitat->GetCurrentLarvalCapacity() - habitat->GetTotalLarvaCount( CURRENT_TIME_STEP );
                egg_crowding_correction_per_species[ vp->get_SpeciesID() ] += habitat->GetEggCrowdingCorrection();
            }
#endif
        }

#ifndef EVERY_VECTOR
        int total_count = adult_count + infected_count + infectious_count ;

        GetOutputStream() << time
                   << "," << nodeId 
                   << "," << pop 
                   << "," << total_count 
                   << "," << adult_count 
                   << "," << infected_count 
                   << "," << infectious_count 
                   << "," << male_count 
                   << "," << new_eggs_count 
                   << "," << indoor_bites_count 
                   << "," << outdoor_bites_count
                   << "," << migration_count_local
                   << "," << migration_count_regional
                   ;

        for( auto sp : species_list )
        {
            GetOutputStream() << "," << available_habitat_per_species[ sp ] ;
        }
        for( auto sp : species_list )
        {
            GetOutputStream() << "," << egg_crowding_correction_per_species[ sp ] ;
        }
        GetOutputStream() << endl;
#endif

        migration_count_local = 0 ;
        migration_count_regional = 0 ;
    }

    bool ReportVectorStats::notifyOnEvent( IIndividualHumanEventContext *context, 
                                            const std::string& StateChange )
    {
        //// iindividual context for suid
        //IIndividualHumanContext * iindividual = NULL;
        //if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanContext), (void**)&iindividual) )
        //{
        //    throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanContext", "IIndividualHumanEventContext");
        //}

        //float mc_weight = context->GetMonteCarloWeight();

        //    NonDiseaseDeaths[cd4_stage][care_stage] += mc_weight;

        return true;
    }

    void ReportVectorStats::LogVectorMigration( ISimulationContext* pSim, 
                                                float currentTime, 
                                                const suids::suid& nodeSuid, 
                                                IVectorCohort* pvc )
    {
        //IVectorCohortIndividual * pivci = NULL;
        //if (s_OK != pvc->QueryInterface(GET_IID(IVectorCohortIndividual), (void**)&pivci) )
        //{
        //    throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pvc", "IVectorCohortIndividual", "IVectorCohort");
        //}

        IMigrate * pim = NULL;
        if (s_OK != pvc->QueryInterface(GET_IID(IMigrate), (void**)&pim) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pvc", "IMigrate", "IVectorCohort");
        }

        //long vci_id = pivci->GetID();
        //int to_node_id = pim->GetMigrationDestination().data ;
        int mig_type = pim->GetMigrationType() ;
        //VectorStateEnum::Enum state = pivci->GetState();
        //std::string species = pivci->GetSpecies();
        //float age = pivci->GetAge();

        if( mig_type == MigrationType::LOCAL_MIGRATION )
            migration_count_local++ ;
        else if( mig_type == MigrationType::REGIONAL_MIGRATION )
            migration_count_regional++ ;

    }


}