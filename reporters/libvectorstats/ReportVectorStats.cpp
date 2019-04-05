/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
#include "ReportUtilities.h"
#include "INodeContext.h"
#include "IdmDateTime.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportVectorStats" ) // <<< Name of this file

namespace Kernel
{
// ----------------------------------------
// --- ReportVectorStats Methods
// ----------------------------------------

    ReportVectorStats::ReportVectorStats()
        : ReportVectorStats( "ReportVectorStats.csv" )
    {
    }

    ReportVectorStats::ReportVectorStats( const std::string& rReportName )
        : BaseTextReportEvents( rReportName )
        , migration_count_local()
        , migration_count_regional()
        , species_list()
        , stratify_by_species(false)
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportVectorStats::~ReportVectorStats()
    {
    }

    bool ReportVectorStats::Configure( const Configuration * inputJson )
    {
        if( inputJson->Exist( "Species_List" ) )
        {
            initConfigTypeMap( "Species_List", &species_list, "The species to include information on." );
        }
        else
        {
            species_list.push_back( "arabiensis" );
            species_list.push_back( "funestus" );
            species_list.push_back( "gambiae" );
        }

        initConfigTypeMap( "Stratify_By_Species", &stratify_by_species, "If true data will break out each the species for each node", false );

        bool ret = JsonConfigurable::Configure( inputJson );

        // Manually push required events into the eventTriggerList
        //eventTriggerList.push_back( EventTrigger::HIVNewlyDiagnosed );
        
        if( ret )
        {
        }
        return ret;
    }

    void ReportVectorStats::UpdateEventRegistration( float currentTime, 
                                                     float dt, 
                                                     std::vector<INodeEventContext*>& rNodeEventContextList,
                                                     ISimulationEventContext* pSimEventContext )
    {
        BaseTextReportEvents::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
    }

    std::string ReportVectorStats::GetHeader() const
    {
        std::stringstream header ;

        header << "Time"                << ", "
               << "NodeID"              << ", ";

        if( stratify_by_species )
        {
            header << "Species" << ", ";
        }

        header << "Population"          << ", "
               << "VectorPopulation"    << ", "
               << "AdultCount"          << ", "
               << "InfectedCount"       << ", "
               << "InfectiousCount"     << ", "
               << "MaleCount"           << ", "
               << "NewEggsCount"        << ", "
               << "IndoorBitesCount"    << ", "
               << "OutdoorBitesCount"   << ", "
               << "NewAdults"           << ", "
               << "DiedBeforeFeeding"          << ", "
               << "DiedDuringFeedingIndoor"    << ", "
               << "DiedDuringFeedingOutdoor"   << ", "
               << "MigrationFromCountLocal"    << ", "
               << "MigrationFromCountRegional"
        ;

        if( stratify_by_species )
        {
            header << ", " << "AvailableHabitat";
            header << ", " << "EggCrowdingCorrection";
        }
        else
        {
            for( auto sp : species_list )
            {
                header << ", " << sp << "_AvailableHabitat";
            }
            for( auto sp : species_list )
            {
                header << ", " << sp << "_EggCrowdingCorrection";
            }
        }

        return header.str();
    }

    void ReportVectorStats::LogNodeData( Kernel::INodeContext* pNC )
    {
        auto time      = pNC->GetTime().time ;
        auto nodeId    = pNC->GetExternalID();
        auto node_suid = pNC->GetSuid();
        auto pop       = pNC->GetStatPop();
        
        INodeVector * pNodeVector = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pNodeVector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext");
        }

        uint32_t adult_count = 0 ;
        uint32_t infected_count = 0 ;
        uint32_t infectious_count = 0 ;
        uint32_t male_count = 0 ;
        uint32_t new_eggs_count = 0 ;
        uint32_t indoor_bites_count = 0 ;
        uint32_t outdoor_bites_count = 0 ;
        uint32_t new_adults = 0;
        uint32_t dead_before = 0;
        uint32_t dead_indoor = 0;
        uint32_t dead_outdoor = 0;
        uint32_t total_migration_count_local = 0;
        uint32_t total_migration_count_regional = 0;
        std::map<std::string,float> available_habitat_per_species ;
        std::map<std::string,float> egg_crowding_correction_per_species ;

        for( auto sp : species_list )
        {
            available_habitat_per_species.insert( std::make_pair( sp, 0.0f ) );
            egg_crowding_correction_per_species.insert( std::make_pair( sp, 0.0f ) );
        }

        ResetOtherCounters();

        const VectorPopulationReportingList_t& vector_pop_list = pNodeVector->GetVectorPopulationReporting();
        for( auto vp : vector_pop_list )
        {
            const std::string& species = vp->get_SpeciesID();

            if( stratify_by_species )
            {
                adult_count = 0;
                infected_count = 0;
                infectious_count = 0;
                male_count = 0;
                new_eggs_count = 0;
                indoor_bites_count = 0;
                outdoor_bites_count = 0;
                new_adults = 0;
                dead_before = 0;
                dead_indoor = 0;
                dead_outdoor = 0;

                ResetOtherCounters();
            }
            adult_count         += vp->getAdultCount();
            infected_count      += vp->getInfectedCount(nullptr);
            infectious_count    += vp->getInfectiousCount(nullptr);
            male_count          += vp->getMaleCount();
            new_eggs_count      += vp->getNewEggsCount();
            indoor_bites_count  += vp->GetHBRByPool( VectorPoolIdEnum::INDOOR_VECTOR_POOL );
            outdoor_bites_count += vp->GetHBRByPool( VectorPoolIdEnum::OUTDOOR_VECTOR_POOL );
            new_adults          += vp->getNewAdults();
            dead_before         += vp->getNumDiedBeforeFeeding();
            dead_indoor         += vp->getNumDiedDuringFeedingIndoor();
            dead_outdoor        += vp->getNumDiedDuringFeedingOutdoor();

            total_migration_count_local    += migration_count_local[ node_suid.data ][ species ];
            total_migration_count_regional += migration_count_regional[ node_suid.data ][ species ];

            for( auto habitat : vp->GetHabitats() )
            {
                available_habitat_per_species[ species ] += habitat->GetCurrentLarvalCapacity() - habitat->GetTotalLarvaCount( CURRENT_TIME_STEP );
                egg_crowding_correction_per_species[ species ] += habitat->GetEggCrowdingCorrection();
            }

            CollectOtherData( vp );

            if( stratify_by_species )
            {
                int total_count = adult_count + infected_count + infectious_count;

                GetOutputStream() << time
                    << "," << nodeId
                    << "," << species
                    << "," << pop
                    << "," << total_count
                    << "," << adult_count
                    << "," << infected_count
                    << "," << infectious_count
                    << "," << male_count
                    << "," << new_eggs_count
                    << "," << indoor_bites_count
                    << "," << outdoor_bites_count
                    << "," << new_adults
                    << "," << dead_before
                    << "," << dead_indoor
                    << "," << dead_outdoor
                    << "," << migration_count_local[ node_suid.data ][ species ]
                    << "," << migration_count_regional[ node_suid.data ][ species ]
                    ;
                GetOutputStream() << "," << available_habitat_per_species[ species ] ;
                GetOutputStream() << "," << egg_crowding_correction_per_species[ species ] ;

                WriteOtherData();

                GetOutputStream() << endl;

                migration_count_local[ node_suid.data ][ species ] = 0;
                migration_count_regional[ node_suid.data ][ species ] = 0;
            }
        }

        if( !stratify_by_species )
        {
            int total_count = adult_count + infected_count + infectious_count;

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
                << "," << new_adults
                << "," << dead_before
                << "," << dead_indoor
                << "," << dead_outdoor
                << "," << total_migration_count_local
                << "," << total_migration_count_regional
                ;
            for( auto sp : species_list )
            {
                GetOutputStream() << "," << available_habitat_per_species[ sp ] ;
            }
            for( auto sp : species_list )
            {
                GetOutputStream() << "," << egg_crowding_correction_per_species[ sp ] ;
            }
            WriteOtherData();
            GetOutputStream() << endl;
        }
        migration_count_local[ node_suid.data ].clear();
        migration_count_regional[ node_suid.data ].clear();
    }

    bool ReportVectorStats::notifyOnEvent( IIndividualHumanEventContext *context, 
                                            const EventTrigger& trigger )
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
        IMigrate * pim = NULL;
        if (s_OK != pvc->QueryInterface(GET_IID(IMigrate), (void**)&pim) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pvc", "IMigrate", "IVectorCohort");
        }

        int mig_type = pim->GetMigrationType();
        const std::string& species = pvc->GetSpecies();

        if( mig_type == MigrationType::LOCAL_MIGRATION )
            migration_count_local[ nodeSuid.data ][species]++ ;
        else if( mig_type == MigrationType::REGIONAL_MIGRATION )
            migration_count_regional[ nodeSuid.data ][species]++ ;
    }
}