/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "Debug.h"
#include "FileSystem.h"
#include "ReportHIVByAgeAndGender.h"
#include "NodeHIV.h"
#include "IHIVInterventionsContainer.h"
#include "ISimulation.h"
#include "SimulationSTI.h" // for base_year
#include "IIndividualHumanHIV.h"
#include "SusceptibilityHIV.h"
#include "EventTrigger.h"
#include "NodeEventContext.h"
#include "Properties.h"

static const char* _module = "ReportHIVByAgeAndGender";

#define MAX_VALUES_PER_BIN (100)

namespace Kernel 
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(ReportHIVByAgeAndGender,ReportHIVByAgeAndGender)

    ReportHIVByAgeAndGender::ReportHIVByAgeAndGender( const ISimulation * parent, float hivPeriod )
        : BaseTextReportEvents("ReportHIVByAgeAndGender.csv")
        , report_hiv_half_period( hivPeriod / 2 )
        , next_report_time(report_hiv_half_period)
        , doReport( false )
        , _parent( parent )
        , startYear(0.0)
        , stopYear(FLT_MAX)
        , is_collecting_data(false)
        , is_collecting_circumcision_data(false)
        , is_collecting_hiv_data(false)
        , is_collecting_ip_data(false)
        , stratify_infected_by_CD4(false)
        , name_of_intervention_to_count()
        , ip_key_list()
        , ip_key_value_list_map()
        , map_key_constants()
    {
        eventTriggerList.push_back( "DiseaseDeaths" );
        eventTriggerList.push_back( "NonDiseaseDeaths" );
        eventTriggerList.push_back( "NewInfectionEvent" );
        eventTriggerList.push_back( "HIVTestedPositive" );
        eventTriggerList.push_back( "HIVTestedNegative" );
    }

    bool ReportHIVByAgeAndGender::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Start_Year", &startYear, Report_HIV_ByAgeAndGender_Start_Year_DESC_TEXT, MIN_YEAR, MAX_YEAR, MIN_YEAR );
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Stop_Year",  &stopYear,  Report_HIV_ByAgeAndGender_Stop_Year_DESC_TEXT,  MIN_YEAR, MAX_YEAR, MAX_YEAR );
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_Circumcision_Data",  
                           &is_collecting_circumcision_data,  
                           Report_HIV_ByAgeAndGender_Collect_Circumcision_Data_DESC_TEXT,
                           false );
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_HIV_Data",  
                           &is_collecting_hiv_data,  
                           Report_HIV_ByAgeAndGender_Collect_HIV_Data_DESC_TEXT,
                           false );
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_IP_Data",
                           &is_collecting_ip_data,  
                           Report_HIV_ByAgeAndGender_Collect_IP_Data_DESC_TEXT, 
                           false );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Stratify_Infected_By_CD4",
                           &stratify_infected_by_CD4,  
                           Report_HIV_ByAgeAndGender_Stratify_Infected_By_CD4_DESC_TEXT, 
                           false );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Event_Counter_List",
                           &event_list,  
                           Report_HIV_ByAgeAndGender_Event_Counter_List_DESC_TEXT );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Has_Intervention_With_Name",
                           &name_of_intervention_to_count,  
                           Report_HIV_ByAgeAndGender_Has_Intervention_With_Name_DESC_TEXT, 
                           "" );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret )
        {
            if( startYear < SimulationSTI::base_year )
            {
                startYear = SimulationSTI::base_year ;
            }
            if( startYear >= stopYear )
            {
                 throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                                                         "Report_HIV_ByAgeAndGender_Start_Year", startYear, 
                                                         "Report_HIV_ByAgeAndGender_Stop_Year", stopYear );
            }

            // -----------------------------------------------------------------------------
            // --- check that the events defined for the report exist in the known event list
            // -----------------------------------------------------------------------------
            EventTrigger tmp;
            for( auto ev : event_list )
            {
                // exception will be thrown if ev not in listed_events
                tmp = ev;

                eventTriggerList.push_back( ev );
            }
        }

        return ret ;
    }

    void ReportHIVByAgeAndGender::UpdateEventRegistration( float currentTime, 
                                                           float dt, 
                                                           std::vector<INodeEventContext*>& rNodeEventContextList )
    {
        // not enforcing simulation to be not null in constructor so one can create schema with it null
        release_assert( _parent );

        float current_year = _parent->GetSimulationTime().Year() ;
        if( !is_collecting_data && (startYear <= current_year) && (current_year < stopYear) )
        {
            BaseTextReportEvents::UpdateEventRegistration( currentTime, dt, rNodeEventContextList );
            is_collecting_data = true ;

            // ------------------------------------------------------------------------
            // --- The idea here is to ensure that as we increase the startYear from the 
            // --- base_year we get the same report times as when startYear = base_year
            // --- The difference between start and base year gives us the number of days
            // --- into the simulation that we think we should be.  It ignores issues with
            // --- the size of dt not ending exactly on integer years.  We subtract the
            // --- dt/2 to deal with rounding errors.  For example, if the half period was 182.5,
            // --- start_year == base_year, and dt = 30, then next_report_time = 167.5 and
            // --- data would be collected at 180.  However, for the next update
            // --- next_report_time would be 350 and the update would occur at 360.
            // ------------------------------------------------------------------------
            next_report_time = DAYSPERYEAR*(startYear - SimulationSTI::base_year) + report_hiv_half_period - dt / 2.0f ;

        }
        else if( is_collecting_data && (_parent->GetSimulationTime().Year() >= stopYear) )
        {
            UnregisterAllNodes();
            is_collecting_data = false ;
        }

        if( is_collecting_data )
        {
            // Figure out when to set doReport to true.  doReport is true for those
            // timesteps where we take a snapshot, i.e., as a function of the
            // half-year offset and full-year periodicity.
            doReport = false;
            LOG_DEBUG_F( "%s: Setting doReport to false\n", __FUNCTION__ );

            if( currentTime >= next_report_time ) 
            {
                next_report_time += report_hiv_half_period;

                LOG_DEBUG_F( "Setting doReport to true .\n" );
                doReport = true;
            }
        }
    }

    bool ReportHIVByAgeAndGender::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return is_collecting_data && doReport; 
    }

    void ReportHIVByAgeAndGender::Initialize( unsigned int nrmSize )
    {
        // ----------------------------------------------------------------------------
        // --- Setup the map_key_constant vector to be used when creating the map_key.
        // --- See GetDataMapKey() for more information
        // ----------------------------------------------------------------------------

        // age will get zero
        if( is_collecting_ip_data )
        {
            const std::vector<IndividualProperty*>& ip_list = IPFactory::GetInstance()->GetIPList();
            for( IndividualProperty* p_ip : ip_list )
            {
                ip_key_list.push_back( p_ip->GetKey().ToString() );
            }

            for( int i = 0 ; i < ip_key_list.size() ; i++ )
            {
                std::string key = ip_key_list[ i ] ;
                std::vector<std::string> value_list ;
                const IPKeyValueContainer& container = IPFactory::GetInstance()->GetIP( key )->GetValues();
                for( IPKeyValue kv : container )
                {
                    value_list.push_back( kv.GetValueAsString() );
                }
                ip_key_value_list_map[ key ] = value_list;

                AddConstant() ;
            }
        }
        AddConstant() ; // is_circumcised
        AddConstant() ; // has_HIV
        AddConstant() ; // gender
        AddConstant() ; // node_id

        BaseTextReportEvents::Initialize( nrmSize );
    }

    void ReportHIVByAgeAndGender::AddConstant()
    {
        // --------------------------------------------------------------------------
        // --- map_key_constants are used to determine the key used in the data_map.
        // --- See GetDataMapKey() for more information
        // --------------------------------------------------------------------------
        if( map_key_constants.size() == 0 )
        {
            map_key_constants.push_back( MAX_VALUES_PER_BIN ) ;
        }
        else
        {
            map_key_constants.push_back( MAX_VALUES_PER_BIN*map_key_constants[ map_key_constants.size()-1 ] ) ;
        }
    }

    std::string ReportHIVByAgeAndGender::GetHeader() const
    {
        std::stringstream header ;
        header << "Year"             << ", "
               << "NodeId"           << ", "
               << "Gender"           << ", " ;
        for( auto key : ip_key_list )
        {
            header << "IP_Key:" << key << ", " ;
        }
        if( is_collecting_circumcision_data )
        {
            header << "IsCircumcised" << ", " ;
        }
        if( is_collecting_hiv_data )
        {
            header << "HasHIV" << ", " ;
        }
        header << "Age"              << ", "
               << "Population"       << ", ";

        if( stratify_infected_by_CD4 )
        {
            header << "Infected CD4 Under 200 (Not On ART)"     << ", "
                   << "Infected CD4 200 To 349 (Not On ART)"    << ", "
                   << "Infected CD4 350 To 499 (Not On ART)"    << ", "
                   << "Infected CD4 500 Plus (Not On ART)"      << ", ";
        }
        header << "Infected" << ", ";

        header << "Newly Infected"             << ", "
               << "On_ART"                     << ", "
               << "Died"                       << ", "
               << "Died_from_HIV"              << ", "
               << "Tested Past Year or On_ART" << ", "
               << "Tested Ever HIVPos"         << ", "
               << "Tested Ever HIVNeg"         << ", "
               << "Tested Positive"            << ", "
               << "Tested Negative"            ;

        if( name_of_intervention_to_count.size() > 0 )
        {
            header << ", " << "HasIntervention(" << name_of_intervention_to_count << ")" ;
        }

        for( auto ev : event_list )
        {
            header << "," << ev ;
        }

        return header.str();
    }

    void ReportHIVByAgeAndGender::LogNodeData( INodeContext* pNC )
    {
        if( (is_collecting_data == false) || (doReport == false) )
        {
            return;
        }
        LOG_DEBUG_F( "%s: doReport = %d\n", __FUNCTION__, doReport );

        float year = _parent->GetSimulationTime().Year();

        int nodeId = pNC->GetExternalID();
        int node_suid_id = pNC->GetSuid().data;

        std::vector<int> key_value_index_list ;
        for( int i = 0 ; i < ip_key_list.size() ; i++ )
        {
            key_value_index_list.push_back( 0 );
        }

        for( int gender = 0; gender < Gender::Enum::COUNT; gender++ ) 
        {
            // ----------------------------------------------------------------------
            // --- The IP loop will iterate over the values of each key.
            // --- Since the number of keys and values per key is not known
            // --- until runtime, we use a 'while' loop and the key_value_index_list
            // --- to loop for each key and the value of each key.
            // ----------------------------------------------------------------------
            bool done_with_ip = false ;
            while( !done_with_ip )
            {
                // -------------------------------------------------------------
                // --- if we are not collecting HIV data, then we just
                // --- want to go through the 'for' loop once.
                // -------------------------------------------------------------
                int num_hiv = Yes_No::COUNT ;
                if( !is_collecting_hiv_data )
                {
                    num_hiv = 1 ;
                }

                for( int hiv_bin = 0; hiv_bin < num_hiv; hiv_bin++ ) 
                {
                    // -------------------------------------------------------------
                    // --- if we are not collecting circumcision data, then we just
                    // --- want to go through the 'for' loop once.
                    // -------------------------------------------------------------
                    int num_circ = Yes_No::COUNT ;
                    if( !is_collecting_circumcision_data )
                    {
                        num_circ = 1 ;
                    }

                    for( int circumcision_bin = 0; circumcision_bin < num_circ; circumcision_bin++ ) 
                    {
                        // Do not report data for circumcised women
                        if( gender == 1 && circumcision_bin == Yes_No::YES )
                        {
                            continue;
                        }

                        for( int age_bin = 0; age_bin < MAX_AGE; age_bin++ ) 
                        {
                                uint64_t map_key = GetDataMapKey( node_suid_id, gender, age_bin, circumcision_bin, hiv_bin, key_value_index_list );
                            ReportData rd ;
                            if( data_map.count( map_key ) > 0 )
                            {
                                rd = data_map[ map_key ] ;
                            }
                            GetOutputStream() << year
                                              << "," << nodeId 
                                              << "," << gender ;// Kernel::Gender::pairs::lookup_key(gender)
                            for( int kvi = 0 ; kvi < key_value_index_list.size() ; kvi++ )
                            {
                                std::string key = ip_key_list[ kvi ] ;
                                std::string value = ip_key_value_list_map[ key ][ key_value_index_list[kvi] ] ;
                                GetOutputStream() << "," << value ;
                            }
                            if( is_collecting_circumcision_data )
                            {
                                GetOutputStream() << "," << circumcision_bin ;
                            }
                            if( is_collecting_hiv_data )
                            {
                                GetOutputStream() << "," << hiv_bin ;
                            }

                            GetOutputStream() << "," << age_bin
                                              << "," << rd.population;

                            if( stratify_infected_by_CD4 )
                            {
                                GetOutputStream() << "," << rd.infected_noART_cd4_under_200
                                                  << "," << rd.infected_noART_cd4_200_to_350
                                                  << "," << rd.infected_noART_cd4_350_to_500
                                                  << "," << rd.infected_noART_cd4_above_500;
                            }
                            GetOutputStream() << "," << rd.infected;


                            GetOutputStream() << "," << rd.newly_infected
                                              << "," << rd.on_ART
                                              << "," << rd.newly_died
                                              << "," << rd.newly_died_from_HIV
                                              << "," << rd.tested_past_year_or_onART
                                              << "," << rd.tested_ever_HIVpos
                                              << "," << rd.tested_ever_HIVneg
                                              << "," << rd.tested_positive
                                              << "," << rd.tested_negative;

                            if( name_of_intervention_to_count.size() > 0 )
                            {
                                GetOutputStream() << "," << rd.has_intervention;
                            }

                            for( auto ev : event_list )
                            {
                                GetOutputStream() << "," << rd.event_counter_map[ ev ];
                            }

                            GetOutputStream() << endl;
                        } // end for age
                    } // end for circ
                } // end for hiv
                // ------------------------------------------------
                // --- Increment the indexes for the next IP value
                // ------------------------------------------------
                done_with_ip = GetNextIP( key_value_index_list );
            }
        }
    }

    void ReportHIVByAgeAndGender::EndTimestep( float currentTime, float dt )
    {
        BaseTextReportEvents::EndTimestep( currentTime, dt );
        if( is_collecting_data && doReport )
        {
            data_map.clear();
        }
    }

    void ReportHIVByAgeAndGender::LogIndividualData( IIndividualHuman* individual )
    {
        LOG_DEBUG_F( "%s: doReport = %d\n", __FUNCTION__, doReport );

        IIndividualHumanHIV* hiv_individual = nullptr;
        if( individual->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&hiv_individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHIV", "IndividualHuman" );
        }

        IHIVMedicalHistory * med_parent = nullptr;
        if ( individual->GetInterventionsContext()->QueryInterface(GET_IID(IHIVMedicalHistory), (void**)&med_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "individual->GetInterventionsContext()", 
                                           "IHIVMedicalHistory",
                                           "IIndividualHumanInterventionsContext" );
        }

        float mc_weight = individual->GetMonteCarloWeight();

        bool isInfected = hiv_individual->HasHIV();
        bool isOnART = hiv_individual->GetHIVInterventionsContainer()->OnArtQuery();

        bool testedPastYear = med_parent->EverTestedPastYear();
        bool testedEver = med_parent->EverTested();

        uint64_t map_key = GetDataMapKey( individual->GetEventContext() );
        if( data_map.count( map_key ) == 0 )
        {
            data_map.insert( std::make_pair( map_key, ReportData() ) );
        }

        data_map[ map_key ].population += mc_weight;

        if( isInfected )
        {
            if( stratify_infected_by_CD4 )
            {
                // Exclude those on ART as they're necessarily infected and shouldn't be 
                //  included in HIV burden calculations for which these strata are intended
                if( !isOnART )
                {
                    float cd4 = hiv_individual->GetHIVSusceptibility()->GetCD4count();
                    if( cd4 < 200 )
                        data_map[ map_key ].infected_noART_cd4_under_200 += mc_weight;
                    else if(cd4 < 350 )
                        data_map[ map_key ].infected_noART_cd4_200_to_350 += mc_weight;
                    else if(cd4 < 500 )
                        data_map[ map_key ].infected_noART_cd4_350_to_500 += mc_weight;
                    else
                        data_map[ map_key ].infected_noART_cd4_above_500 += mc_weight;
                }
            }
            data_map[ map_key ].infected += mc_weight;
        }

        if( isOnART )
            data_map[ map_key ].on_ART += mc_weight;

        if( testedPastYear || isOnART )
            data_map[ map_key ].tested_past_year_or_onART += mc_weight;

        if( testedEver )
        {
            if( isInfected )
            {
                data_map[ map_key ].tested_ever_HIVpos += mc_weight;
            }
            else
            {
                data_map[ map_key ].tested_ever_HIVneg += mc_weight;
            }
        }

        if( name_of_intervention_to_count.length() > 0 )
        {
            auto existing_vaccines = individual->GetInterventionsContext()->GetInterventionsByName( name_of_intervention_to_count );
            if( existing_vaccines.size() > 0 )
            {
                data_map[ map_key ].has_intervention += mc_weight;
            }
        }
    }

    bool ReportHIVByAgeAndGender::notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange )
    {
        LOG_DEBUG_F( "Individual %d experienced event %s\n",
                     context->GetSuid().data,
                     StateChange.c_str()
                   );

        uint64_t map_key = GetDataMapKey( context );
        if( data_map.count( map_key ) == 0 )
        {
            data_map.insert( std::make_pair( map_key, ReportData() ) );
        }

        float mc_weight = context->GetMonteCarloWeight();

        if( StateChange == "DiseaseDeaths" ) 
        {
            data_map[ map_key ].newly_died          += mc_weight;
            data_map[ map_key ].newly_died_from_HIV += mc_weight;
        }
        else if( StateChange == "NonDiseaseDeaths" )
        {
            data_map[ map_key ].newly_died += mc_weight;
        }
        else if( StateChange == "NewInfectionEvent" )
        {
            data_map[ map_key ].newly_infected += mc_weight;
        }
        else if( StateChange == "HIVTestedPositive" )
        {
            data_map[ map_key ].tested_positive += mc_weight;
        }
        else if( StateChange == "HIVTestedNegative" )
        {
            data_map[ map_key ].tested_negative += mc_weight;
        }
        else if( std::find( event_list.begin(), event_list.end(), StateChange ) != event_list.end() )
        {
             data_map[ map_key ].event_counter_map[ StateChange ] += mc_weight;
        }
 
        return true;
    }

    bool ReportHIVByAgeAndGender::GetNextIP( std::vector<int>& rKeyValueIndexList )
    {
        // ----------------------------------------------------
        // --- Increment the indexes and determine if we are 
        // --- done looping over all of the values for each key.
        // ----------------------------------------------------
        bool is_done = true ;
        bool increment_next = true ;
        for( int kvi = rKeyValueIndexList.size() -1 ; increment_next && (kvi >= 0) ; kvi-- )
        {
            rKeyValueIndexList[ kvi ]++ ;
            is_done = false ;

            std::string key = ip_key_list[ kvi ] ;
            if( rKeyValueIndexList[ kvi ] >= ip_key_value_list_map[ key ].size() )
            {
                rKeyValueIndexList[ kvi ] = 0 ;
                if( kvi == 0 )
                {
                    is_done = true ;
                }
            }
            else
            {
                increment_next = false ;
            }
        }
        return is_done ;
    }

    uint64_t ReportHIVByAgeAndGender::GetDataMapKey( IIndividualHumanEventContext* context )
    {
        int gender = context->GetGender() == Gender::MALE ? 0 : 1;
        int age_bin = (int)floor( (std::min)((double) MAX_AGE-1, context->GetAge()/(float)DAYSPERYEAR) );

        IIndividualHumanSTI* sti_individual = NULL;
        if( context->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&sti_individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualSTI", "IndividualHuman" );
        }
        int circumcision_bin = (is_collecting_circumcision_data && sti_individual->IsCircumcised()) ? 1 : 0;

        IIndividualHumanHIV* hiv_individual = NULL;
        if( context->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&hiv_individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHIV", "IndividualHuman" );
        }
        int hiv_bin = (is_collecting_hiv_data && hiv_individual->HasHIV()) ? 1 : 0;

        auto* pProp = context->GetProperties();

        // --------------------------------------------------------------------------
        // --- Based on the individual's values for each Indivdual Property Key find 
        // --- the indexes of those values in the value list for the key.
        // --------------------------------------------------------------------------
        std::vector<int> key_value_list_index ;
        for( auto key : ip_key_list )
        {
            std::string value = pProp->at( key );
            int index = std::find( ip_key_value_list_map[ key ].begin(), ip_key_value_list_map[ key ].end(), value ) - ip_key_value_list_map[ key ].begin() ;
            key_value_list_index.push_back( index );
        }
        int node_suid_id = context->GetNodeEventContext()->GetId().data;

        uint64_t map_key = GetDataMapKey( node_suid_id, gender, age_bin, circumcision_bin, hiv_bin, key_value_list_index );
        return map_key ;
    }

    uint64_t ReportHIVByAgeAndGender::GetDataMapKey( int nodeSuidIndex, int genderIndex, int ageIndex, int circIndex, int hivIndex, const std::vector<int>& rKeyValueIndexList )
    {
        // ------------------------------------------------------------------------------
        // --- I used factors of MAX_VALUES_PER_BIN-100 in the map_key_constant so that one could look at
        // --- the decimal value to determine the what the indexing would be if this were 
        // --- truly a multi-dimensional array.
        // ------------------------------------------------------------------------------
        release_assert( nodeSuidIndex < MAX_VALUES_PER_BIN );
        release_assert( genderIndex   < MAX_VALUES_PER_BIN );
        release_assert( ageIndex      < MAX_VALUES_PER_BIN );
        release_assert( circIndex     < MAX_VALUES_PER_BIN );
        release_assert( hivIndex      < MAX_VALUES_PER_BIN );

        uint64_t map_key = 0 ;

        map_key += ageIndex ;
        for( int kvi = 0 ; kvi < rKeyValueIndexList.size() ; kvi++ )
        {
            release_assert( rKeyValueIndexList[ kvi ] < MAX_VALUES_PER_BIN );
            map_key += map_key_constants[ kvi ] * rKeyValueIndexList[ kvi ] ;
        }
        map_key += map_key_constants[ rKeyValueIndexList.size()+0 ] * circIndex;
        map_key += map_key_constants[ rKeyValueIndexList.size()+1 ] * hivIndex;
        map_key += map_key_constants[ rKeyValueIndexList.size()+2 ] * genderIndex;
        map_key += map_key_constants[ rKeyValueIndexList.size()+3 ] * nodeSuidIndex;

        return map_key;
    }

}

