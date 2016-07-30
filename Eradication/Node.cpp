/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "NodeDemographics.h"
#include "Node.h"

#include <iostream>
#include <algorithm>
#include <numeric>

#include "Debug.h" // for release_assert
#include "MathFunctions.h" // for fromDistribution
#include "Common.h"
#include "Contexts.h"
#include "Exceptions.h"
#include "InterventionEnums.h"
#include "Types.h"
#include "NodeEventContext.h"
#include "NodeEventContextHost.h"
#include "TransmissionGroupMembership.h"
#include "TransmissionGroupsFactory.h"
#include "Simulation.h"
#include "SimulationConfig.h"
#include "StatusReporter.h" // for initialization progress
#include "StrainIdentity.h"
#include "IInfectable.h"
#include "Instrumentation.h"
#include "FileSystem.h"
#include "IMigrationInfo.h"
#include "Individual.h"
#include "Serialization.h"
#include "IdmMpi.h"

static const char* _module = "Node";

using namespace json;

template <typename T, std::size_t N>
inline std::size_t sizeof_array( T (&)[N] ) {
   return N;
}

#include "Properties.h"

namespace Kernel
{
    const char * Node::_age_bins_key = "Age_Bin"; // move to Properties.h
    const char * Node::transitions_dot_json_filename = "transitions.json";
    std::string Node::getAgeBinPropertyNameFromIndex(
        const NodeDemographics& demo,
        unsigned int idx
    )
    {
        release_assert( idx>0 );
        float min_age = float(demo[idx-1].AsDouble());
        float max_age = float(demo[idx].AsDouble());
        std::ostringstream retMsg;
        retMsg << "Age_Bin_Property_From_" << min_age << "_To_" << max_age;
        return retMsg.str();
    }

    void Node::checkValidIPValue( const std::string& key, const std::string& to_value )
    {
        LOG_DEBUG_F( "%s: key=%s, value=%s\n", __FUNCTION__, key.c_str(), to_value.c_str() );
        bool good = false;
        release_assert( distribs.find( key ) != distribs.end() );
        for (const auto& pair : distribs.at(key))
        {
            if( pair.second == to_value )
            {
                good = true;
            }
        }

        if( good == false )
        {
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, ( std::string( "Bad to_value in Individual_Property transitions: " ) + to_value ).c_str() );
        }
    }

    void Node::TestOnly_ClearProperties()
    {
        base_distribs.clear();
    }

    void Node::TestOnly_AddPropertyKeyValue( const char* key, const char* value )
    {
        base_distribs[ key ].insert( make_pair( 0.0f, value ) );
    }

    std::vector<std::string> Node::GetIndividualPropertyKeyList()
    {
        std::vector<std::string> key_list ;
        for( auto entry : base_distribs )
        {
            key_list.push_back( entry.first );
        }
        return key_list ;
    }

    std::vector<std::string> Node::GetIndividualPropertyValuesList( const std::string& rKey )
    {
        if( base_distribs.find( rKey ) == base_distribs.end() )
        {
            // failed to find this property key in the main map.
            std::ostringstream msg;
            msg << "Failed to find property key " << rKey << " in individual properties specified in demographics file." << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,  msg.str().c_str() );
        }

        std::vector<std::string> values ;
        for( auto& prop_entry : base_distribs[ rKey ] )
        {
            values.push_back( prop_entry.second );
        }
        return values ;
    }

    void Node::VerifyPropertyDefinedInDemographics( const std::string& rKey, const std::string& rVal )
    {
#if defined(_DLLS_)
        return;
#endif
        if( base_distribs.find( rKey ) != base_distribs.end() )
        {
            bool found = false;
            for( auto& prop_entry : base_distribs[ rKey ] )
            {
                auto val = prop_entry.second;
                if( rVal == val )
                {
                    found = true;
                    break;
                }
            }
            if( found == false )
            {
                // error: value not found
                std::ostringstream msg;
                msg << "Failed to find property restriction value " << rVal << " in individual properties specified in demographics file." << std::endl;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,  msg.str().c_str() );
            }
        }
        else
        {
            // failed to find this property key in the main map.
            std::ostringstream msg;
            msg << "Failed to find property restriction key " << rKey << " in individual properties specified in demographics file." << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,  msg.str().c_str() );
        }
    }

    void Node::VerifyPropertyDefined( const std::string& rKey, const std::string& rVal ) const
    {
        // --------------------------------------------------------------------
        // --- We have a static version of this method because it is using
        // --- the static variables.  We have a virtual method of this check
        // --- because we want the DLL's to have access to this static variable.
        // --------------------------------------------------------------------
        VerifyPropertyDefinedInDemographics( rKey, rVal );
    }

    void Node::convertTransitions(
        const NodeDemographics &trans,
        json::Object& tx_camp,
        const std::string& prop_key
    )
    {
        DemographicsContext::using_compiled_demog = false;
        for( int idx = 0; idx<trans.get_array().size(); idx++ )
        {
            json::Object new_event;
            json::QuickBuilder new_event_qb( new_event );
            new_event[ "class" ] = json::String( "CampaignEvent" );

            json::Object new_sub_event = json::Object();
            new_sub_event[ "class" ] = json::String( "StandardInterventionDistributionEventCoordinator" );

            json::Object new_ic = json::Object();
            json::QuickBuilder new_ic_qb = json::QuickBuilder( new_ic );

            new_event_qb[ "Event_Coordinator_Config" ] = new_sub_event;
            // This boilerplate stuff should come from Use_Defaults(!)
            new_event_qb[ "Event_Coordinator_Config" ][ "Dont_Allow_Duplicates" ] = json::Number( 0 );
            new_event_qb[ "Event_Coordinator_Config" ][ "Number_Distributions" ] = json::Number( -1.0 );
            new_event_qb[ "Event_Coordinator_Config" ][ "Number_Repetitions" ] = json::Number( 1.0 );
            new_event_qb[ "Event_Coordinator_Config" ][ "Property_Restrictions" ] = json::Array();
            new_event_qb[ "Event_Coordinator_Config" ][ "Target_Demographic" ] = json::String( "Everyone" );
            new_event_qb[ "Event_Coordinator_Config" ][ "Timesteps_Between_Repetitions" ] = json::Number( 0 );
            new_event_qb[ "Event_Coordinator_Config" ][ "Target_Residents_Only" ] = json::Number( 0 );
            new_event_qb[ "Event_Coordinator_Config" ][ "Include_Arrivals" ] = json::Number( 0 );
            new_event_qb[ "Event_Coordinator_Config" ][ "Include_Departures" ] = json::Number( 0 );

            new_event_qb[ "Nodeset_Config" ] = json::Object();
            new_event_qb[ "Nodeset_Config" ][ "class" ] = json::String( "NodeSetAll" );

            trans[ idx ].validateIPTransition();

            std::string from_value = trans[idx][ "From" ].AsString();
            LOG_DEBUG_F( "from_value = %s\n", from_value.c_str() );

            // Don't use property_restrictions to target age_bin transitions
            if( from_value != "NULL" && prop_key != _age_bins_key ) // Initial transitions for Age_Bins have no original property
            {
                // this is implemented as a property restriction in the event coord
                new_event_qb[ "Event_Coordinator_Config" ][ "Property_Restrictions" ] = json::Array();
                new_event_qb[ "Event_Coordinator_Config" ][ "Property_Restrictions" ][0] = json::String( prop_key + ":" + from_value );
            } 

            std::string to_value = trans[idx][ "To" ].AsString();
            LOG_DEBUG_F( "to_value = %s\n", to_value.c_str() );
            checkValidIPValue( prop_key, to_value );
            double coverage_value = trans[idx][ "Coverage" ].AsDouble();
            LOG_DEBUG_F( "coverage_value = %f\n", coverage_value );
            new_event_qb[ "Event_Coordinator_Config" ][ "Demographic_Coverage" ] = json::Number( coverage_value );

            double probability = trans[idx][ IP_PROBABILITY_KEY ].AsDouble();
            LOG_DEBUG_F( "probability = %f\n", probability );
            std::string type = trans[idx][ "Type" ].AsString();
            LOG_DEBUG_F( "type = %s\n", type.c_str() );
            double when_value = trans[idx][ IP_WHEN_KEY ][ "Start" ].AsDouble();
            LOG_DEBUG_F( "when_value = %f\n", when_value );
            double stop_time = FLT_MAX;
            if( trans[idx][ IP_WHEN_KEY ].Contains( "Duration" ) )
            {
                stop_time = trans[idx][ IP_WHEN_KEY ][ "Duration" ].AsDouble();
                LOG_DEBUG_F( "stop_time = %f\n", stop_time );
            }
            std::string trigger = "";
            float reversion = 0.0f;
            if( trans[idx].Contains( IP_REVERSION_KEY ) )
            {
                reversion = trans[idx][ IP_REVERSION_KEY ].AsDouble();
            }
            if( type == "At_Timestep" )
            {
                new_event[ "Start_Day" ] = json::Number( when_value );
                new_ic_qb[ "class" ] = json::String( "PropertyValueChanger" );
                new_ic_qb[ "Dont_Allow_Duplicates" ] = json::Number( 0 );
                new_ic_qb[ "Target_Property_Key" ] = json::String( prop_key );
                new_ic_qb[ "Target_Property_Value" ] = json::String( to_value );
                new_ic_qb[ "Daily_Probability" ] = json::Number( probability );
                new_ic_qb[ "Maximum_Duration" ] = json::Number( stop_time );
                new_ic_qb[ "Revert" ] = json::Number( reversion );

                if( when_value == 0 && prop_key == _age_bins_key && to_value.find( "Age_Bin_Property_From_0" ) != std::string::npos )
                {
                    // We need a birth-triggered intervention for this age_bin initialization from age_bin 0.
                    // Add BirthTriggeredIV.
                    json::QuickBuilder new_event_birth = new_event; // better be a deep copy
                    new_event_birth[ "Start_Day" ] = json::Number( 0 );

                    json::Object new_bti = json::Object();
                    json::QuickBuilder new_bti_qb = json::QuickBuilder( new_bti );
                    new_bti_qb[ "class" ] = json::String( "BirthTriggeredIV" );
                    new_bti_qb[ "Dont_Allow_Duplicates" ] = json::Number( 0 );
                    new_bti_qb[ "Demographic_Coverage" ] = json::Number( 1.0 );
                    new_bti_qb[ "Target_Demographic" ] = json::String( "Everyone" );
                    new_bti_qb[ "Target_Residents_Only" ] = json::Number( 0 );
                    new_bti_qb[ "Property_Restrictions" ] = json::Array();
                    new_bti_qb[ "Duration" ] = json::Number( -1.0 );
                    new_bti_qb[ "Actual_IndividualIntervention_Config" ] = new_ic_qb.As<json::Object>();
                    new_event_birth[ "Event_Coordinator_Config" ][ "Intervention_Config" ] = new_bti; // this is CRAP (TBD)
                    ((json::Array&)tx_camp[ "Events" ]).Insert( new_event_birth );
                }
            }
            else if( type == "At_Age" )
            {
                // Add Calendar Intervention
                double age = DAYSPERYEAR * trans[idx][ "Age_In_Years" ].AsDouble();
                new_event[ "Start_Day" ] = json::Number( when_value );
                new_ic_qb[ "class" ] = json::String( "IVCalendar" );
                new_ic_qb[ "Dont_Allow_Duplicates" ] = json::Number( 0 );
                new_ic_qb[ "Dropout" ] = json::Number( 0 );
                new_ic_qb[ "Calendar" ][0][ "Age" ] = json::Number( age );
                new_ic_qb[ "Calendar" ][0][ "Probability" ] = json::Number( 1.0 );
                new_ic_qb[ "Actual_IndividualIntervention_Configs" ][0][ "class" ] = json::String( "PropertyValueChanger" );
                new_ic_qb[ "Actual_IndividualIntervention_Configs" ][0][ "Dont_Allow_Duplicates" ] = json::Number( 0 );
                new_ic_qb[ "Actual_IndividualIntervention_Configs" ][0][ "Target_Property_Key" ] = json::String( prop_key );
                new_ic_qb[ "Actual_IndividualIntervention_Configs" ][0][ "Target_Property_Value" ] = json::String( to_value );
                new_ic_qb[ "Actual_IndividualIntervention_Configs" ][0][ "Daily_Probability" ] = json::Number( probability );
                new_ic_qb[ "Actual_IndividualIntervention_Configs" ][0][ "Maximum_Duration" ] = json::Number( stop_time );
                new_ic_qb[ "Actual_IndividualIntervention_Configs" ][0][ "Revert" ] = json::Number( reversion );

                // Add BirthTriggeredIV.
                json::Element deepCopy = new_event;
                json::QuickBuilder new_event_birth( deepCopy );
                new_event_birth[ "Start_Day" ] = json::Number( 0 );

                json::Object new_bti = json::Object();
                json::QuickBuilder new_bti_qb = json::QuickBuilder( new_bti );
                new_bti_qb[ "class" ] = json::String( "BirthTriggeredIV" );
                new_bti_qb[ "Dont_Allow_Duplicates" ] = json::Number( 0 );
                new_bti_qb[ "Demographic_Coverage" ] = json::Number( 1.0 );
                new_bti_qb[ "Target_Demographic" ] = json::String( "Everyone" );
                new_bti_qb[ "Target_Residents_Only" ] = json::Number( 0 );
                new_bti_qb[ "Property_Restrictions" ] = json::Array();
                new_bti_qb[ "Duration" ] = json::Number( -1.0 );
                new_bti_qb[ "Actual_IndividualIntervention_Config" ] = new_ic_qb.As<json::Object>();
                new_event_birth[ "Event_Coordinator_Config" ][ "Intervention_Config" ] = new_bti; // this is CRAP (TBD)
                ((json::Array&)tx_camp[ "Events" ]).Insert( new_event_birth );

                // Set here after copied for BirthTriggeredIV
                new_event_qb[ "Event_Coordinator_Config" ][ "Target_Demographic" ] = json::String( "ExplicitAgeRanges" );
                double min = 0;
                double max = age/DAYSPERYEAR;
                new_event_qb[ "Event_Coordinator_Config" ][ "Target_Age_Min" ] = json::Number( min );
                new_event_qb[ "Event_Coordinator_Config" ][ "Target_Age_Max" ] = json::Number( max );

            }
            else
            {
                ostringstream msg;
                msg << "type = " << type << ", not understood. Should be At_Timestep, At_Age, or At_Event.";
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            if( trans[idx].Contains( IP_AGE_KEY ) )
            {
                auto age_bounds = trans[idx][ IP_AGE_KEY ];
                if( age_bounds.Contains( "Min" ) )
                {
                    new_event_qb[ "Event_Coordinator_Config" ][ "Target_Demographic" ] = json::String( "ExplicitAgeRanges" );
                    double min = trans[idx][ IP_AGE_KEY ][ "Min" ].AsDouble();
                    double max = MAX_HUMAN_AGE;
                    if( trans[idx][ IP_AGE_KEY ].Contains( "Max" ) )
                    {
                        max = trans[idx][ IP_AGE_KEY ][ "Max" ].AsDouble();
                        if( max == -1 )
                        {
                            max = MAX_HUMAN_AGE;
                        }
                    }
                    new_event_qb[ "Event_Coordinator_Config" ][ "Target_Age_Min" ] = json::Number( min );
                    new_event_qb[ "Event_Coordinator_Config" ][ "Target_Age_Max" ] = json::Number( max );
                }
            }
            new_event_qb[ "Event_Coordinator_Config" ][ "Intervention_Config" ] = new_ic;
            ((json::Array&)tx_camp[ "Events" ]).Insert( new_event );
        }
        DemographicsContext::using_compiled_demog = true;
    }

    // QI stoff in case we want to use it more extensively
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Node,Node)

    //------------------------------------------------------------------
    //   Initialization methods
    //------------------------------------------------------------------

    static const char* keys_whitelist_tmp[] = { "Accessibility", "Geographic", "Place", "Risk", "QualityOfCare", "HasActiveTB"  };

    // <ERAD-291>
    // TODO: Make simulation object initialization more consistent.  Either all pass contexts to constructors or just have empty constructors
    Node::Node(ISimulationContext *_parent_sim, suids::suid _suid)
        : serializationMask(SerializationFlags(uint32_t(SerializationFlags::Population) | uint32_t(SerializationFlags::Parameters)))
        , _latitude(FLT_MAX)
        , _longitude(FLT_MAX)
        , distribs()
        , ind_sampling_type( IndSamplingType::TRACK_ALL )
        , population_density_infectivity_correction(PopulationDensityInfectivityCorrection::CONSTANT_INFECTIVITY)
        , suid(_suid)
        , urban(false)
        , birthrate(DEFAULT_BIRTHRATE)
        , Above_Poverty(DEFAULT_POVERTY_THRESHOLD)
        , individualHumans()
        , home_individual_ids()
        , family_waiting_to_migrate(false)
        , family_migration_destination(suids::nil_suid())
        , family_migration_type(MigrationType::NO_MIGRATION)
        , family_time_until_trip(0.0f)
        , family_time_at_destination(0.0f)
        , family_is_destination_new_home(false)
        , Ind_Sample_Rate(1.0f)
        , transmissionGroups(nullptr)
        , susceptibility_dynamic_scaling(1.0f)
        , localWeather(nullptr)
        , migration_info(nullptr)
        , demographics()
        , demographic_distributions()
        , externalId(0)
        , event_context_host(nullptr)
        , events_from_other_nodes()
        , statPop(0)
        , Infected(0)
        , Births(0.0f)
        , Disease_Deaths(0.0f)
        , new_infections(0.0f)
        , new_reportedinfections(0.0f)
        , Cumulative_Infections(0.0f)
        , Cumulative_Reported_Infections(0.0f)
        , Campaign_Cost(0.0f)
        , Possible_Mothers(0)           // counters starting with this one were only used for old spatial reporting and can probably now be removed
        , mean_age_infection(0.0f)
        , newInfectedPeopleAgeProduct(0.0f)
        , infected_people_prior()
        , infected_age_people_prior()
        , infectionrate(0.0f)
        , mInfectivity(0.0f)
        , parent(nullptr)
        , demographics_birth(false)
        , demographics_gender(false)
        , demographics_other(false)
        , max_sampling_cell_pop(0.0f)
        , sample_rate_birth(0.0f)
        , sample_rate_0_18mo(0.0f)
        , sample_rate_18mo_4yr(0.0f)
        , sample_rate_5_9(0.0f)
        , sample_rate_10_14(0.0f)
        , sample_rate_15_19(0.0f)
        , sample_rate_20_plus(0.0f)
        , sample_rate_immune(0.0f)
        , immune_threshold_for_downsampling(0.0f)
        , prob_maternal_transmission(0.0f)
        , population_density_c50(0.0f)
        , population_scaling_factor(1.0f)
        , maternal_transmission(false)
        , vital_birth(false)
        , vital_birth_dependence(VitalBirthDependence::FIXED_BIRTH_RATE)
        , vital_birth_time_dependence(VitalBirthTimeDependence::NONE)
        , x_birth(0.0f)
        , immunity_dist_type(DistributionFunction::NOT_INITIALIZED)
        , immunity_dist1(0.0f)
        , immunity_dist2(0.0f)
        , risk_dist_type(DistributionFunction::NOT_INITIALIZED)
        , risk_dist1(0.0f)
        , risk_dist2(0.0f)
        , migration_dist_type(DistributionFunction::NOT_INITIALIZED)
        , migration_dist1(0.0f)
        , migration_dist2(0.0f)
        , animal_reservoir_type(AnimalReservoir::NO_ZOONOSIS)
        , infectivity_scaling(InfectivityScaling::CONSTANT_INFECTIVITY)
        , zoonosis_rate(0.0f)
        , routes()
        , whitelist_enabled(true)
        , ipkeys_whitelist( keys_whitelist_tmp, keys_whitelist_tmp+sizeof_array(keys_whitelist_tmp) )
        , infectivity_sinusoidal_forcing_amplitude( 1.0f )
        , infectivity_sinusoidal_forcing_phase( 0.0f )
        , infectivity_boxcar_forcing_amplitude( 1.0f )
        , infectivity_boxcar_start_time( 0.0 )
        , infectivity_boxcar_end_time( 0.0 )
        , birth_rate_sinusoidal_forcing_amplitude( 1.0f )
        , birth_rate_sinusoidal_forcing_phase( 0.0f )
        , birth_rate_boxcar_forcing_amplitude( 1.0f )
        , birth_rate_boxcar_start_time( 0.0 )
        , birth_rate_boxcar_end_time( 0.0 )
    {
        SetContextTo(_parent_sim);  // TODO - this should be a virtual function call, but it isn't because the constructor isn't finished running yet.
// clorton        const char* keys_whitelist_tmp[] = { "Accessibility", "Geographic", "Place", "Risk", "QualityOfCare", "HasActiveTB"  };
// clorton        ipkeys_whitelist = std::set< std::string> ( keys_whitelist_tmp, keys_whitelist_tmp+sizeof_array(keys_whitelist_tmp) );
// clorton        whitelist_enabled = true;
    }

    Node::Node()
        : serializationMask(SerializationFlags(uint32_t(SerializationFlags::Population) | uint32_t(SerializationFlags::Parameters)))
        , _latitude(FLT_MAX)
        , _longitude(FLT_MAX)
        , distribs()
        , ind_sampling_type( IndSamplingType::TRACK_ALL )
        , population_density_infectivity_correction(PopulationDensityInfectivityCorrection::CONSTANT_INFECTIVITY)
        , suid(suids::nil_suid())
        , urban(false)
        , birthrate(DEFAULT_BIRTHRATE)
        , Above_Poverty(DEFAULT_POVERTY_THRESHOLD)
        , individualHumans()
        , home_individual_ids()
        , family_waiting_to_migrate(false)
        , family_migration_destination(suids::nil_suid())
        , family_migration_type(MigrationType::NO_MIGRATION)
        , family_time_until_trip(0.0f)
        , family_time_at_destination(0.0f)
        , family_is_destination_new_home(false)
        , Ind_Sample_Rate(1.0f)
        , transmissionGroups(nullptr)
        , susceptibility_dynamic_scaling(1.0f)
        , localWeather(nullptr)
        , migration_info(nullptr)
        , demographics()
        , demographic_distributions()
        , externalId(0)
        , event_context_host(nullptr)
        , events_from_other_nodes()
        , statPop(0)
        , Infected(0)
        , Births(0.0f)
        , Disease_Deaths(0.0f)
        , new_infections(0.0f)
        , new_reportedinfections(0.0f)
        , Cumulative_Infections(0.0f)
        , Cumulative_Reported_Infections(0.0f)
        , Campaign_Cost(0.0f)
        , Possible_Mothers(0)           // counters starting with this one were only used for old spatial reporting and can probably now be removed
        , mean_age_infection(0.0f)
        , newInfectedPeopleAgeProduct(0.0f)
        , infected_people_prior()
        , infected_age_people_prior()
        , infectionrate(0.0f)
        , mInfectivity(0.0f)
        , parent(nullptr)
        , demographics_birth(false)
        , demographics_gender(false)
        , demographics_other(false)
        , max_sampling_cell_pop(0.0f)
        , sample_rate_birth(0.0f)
        , sample_rate_0_18mo(0.0f)
        , sample_rate_18mo_4yr(0.0f)
        , sample_rate_5_9(0.0f)
        , sample_rate_10_14(0.0f)
        , sample_rate_15_19(0.0f)
        , sample_rate_20_plus(0.0f)
        , sample_rate_immune(0.0f)
        , immune_threshold_for_downsampling(0.0f)
        , prob_maternal_transmission(0.0f)
        , population_density_c50(0.0f)
        , population_scaling_factor(0.0f)
        , maternal_transmission(false)
        , vital_birth(false)
        , vital_birth_dependence(VitalBirthDependence::FIXED_BIRTH_RATE)
        , vital_birth_time_dependence(VitalBirthTimeDependence::NONE)
        , x_birth(0.0f)
        , immunity_dist_type(DistributionFunction::NOT_INITIALIZED)
        , immunity_dist1(0.0f)
        , immunity_dist2(0.0f)
        , risk_dist_type(DistributionFunction::NOT_INITIALIZED)
        , risk_dist1(0.0f)
        , risk_dist2(0.0f)
        , migration_dist_type(DistributionFunction::NOT_INITIALIZED)
        , migration_dist1(0.0f)
        , migration_dist2(0.0f)
        , animal_reservoir_type(AnimalReservoir::NO_ZOONOSIS)
        , infectivity_scaling(InfectivityScaling::CONSTANT_INFECTIVITY)
        , zoonosis_rate(0.0f)
        , routes()
        , whitelist_enabled(true)
        , ipkeys_whitelist( keys_whitelist_tmp, keys_whitelist_tmp+sizeof_array(keys_whitelist_tmp) )
        , infectivity_sinusoidal_forcing_amplitude( 1.0f )
        , infectivity_sinusoidal_forcing_phase( 0.0f )
        , infectivity_boxcar_forcing_amplitude( 1.0f )
        , infectivity_boxcar_start_time( 0.0 )
        , infectivity_boxcar_end_time( 0.0 )
        , birth_rate_sinusoidal_forcing_amplitude( 1.0f )
        , birth_rate_sinusoidal_forcing_phase( 0.0f )
        , birth_rate_boxcar_forcing_amplitude( 1.0f )
        , birth_rate_boxcar_start_time( 0.0 )
        , birth_rate_boxcar_end_time( 0.0 )
    {
        // No more to do here.
    }

    Node::~Node()
    {
        if (suid.data % 10 == 0) LOG_INFO_F("Freeing Node %d \n", suid.data);

        /* Let all of this dangle, we're about to exit the process...
        for (auto individual : individualHumans)
        {
            delete individual;
        }

        individualHumans.clear();
        home_individual_ids.clear();

        if (transmissionGroups) delete transmissionGroups;
        if (localWeather)       delete localWeather;
        if (migration_info)     delete migration_info;

        delete event_context_host;

        for (auto& ndd_pair : demographic_distributions)
        {
            delete ndd_pair.second;
        }

        demographic_distributions.clear();
        */
    }

    float Node::GetLatitudeDegrees()
    {
        if( _latitude == FLT_MAX )
        {
            _latitude  = float(demographics["NodeAttributes"]["Latitude"].AsDouble());
        }
        return _latitude ;
    }

    float Node::GetLongitudeDegrees()
    {
        if( _longitude == FLT_MAX )
        {
            _longitude = float(demographics["NodeAttributes"]["Longitude"].AsDouble());
        }
        return _longitude ;
    }
        
    QueryResult Node::QueryInterface( iid_t iid, void** ppinstance )
    {
        release_assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        ISupports* foundInterface;
        if ( iid == GET_IID(INodeContext)) 
            foundInterface = static_cast<INodeContext*>(this);
        else if ( iid == GET_IID(ISupports) )
            foundInterface = static_cast<ISupports*>(static_cast<INodeContext*>(this));
        else if (iid == GET_IID(IGlobalContext))
            parent->QueryInterface(iid, (void**) &foundInterface);
        else
            foundInterface = nullptr;

        QueryResult status = e_NOINTERFACE;
        if ( foundInterface )
        {
            foundInterface->AddRef();
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

    Node *Node::CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid)
    {
        Node *newnode = _new_ Node(_parent_sim, node_suid);
        newnode->Initialize();

        return newnode;
    }

    bool Node::Configure( const Configuration* config )
    {
        initConfig( "Infectivity_Scale_Type", infectivity_scaling, config, MetadataDescriptor::Enum("infectivity_scaling", Infectivity_Scale_Type_DESC_TEXT, MDD_ENUM_ARGS(InfectivityScaling)) );
        if ((infectivity_scaling == InfectivityScaling::SINUSOIDAL_FUNCTION_OF_TIME) || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Infectivity_Sinusoidal_Forcing_Amplitude", &infectivity_sinusoidal_forcing_amplitude, Sinusoidal_Infectivity_Forcing_Amplitude_DESC_TEXT, 0.0f, 1.0f, 0.0f );
            initConfigTypeMap( "Infectivity_Sinusoidal_Forcing_Phase", &infectivity_sinusoidal_forcing_phase, Sinusoidal_Infectivity_Forcing_Phase_DESC_TEXT, 0.0f, 365.0f, 0.0f );
        }
        if ((infectivity_scaling  == InfectivityScaling::ANNUAL_BOXCAR_FUNCTION) || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Infectivity_Boxcar_Forcing_Amplitude", &infectivity_boxcar_forcing_amplitude, Boxcar_Infectivity_Forcing_Amplitude_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
            initConfigTypeMap( "Infectivity_Boxcar_Forcing_Start_Time", &infectivity_boxcar_start_time, Boxcar_Infectivity_Forcing_Start_Time_DESC_TEXT, 0.0f, 365.0f, 0.0f );
            initConfigTypeMap( "Infectivity_Boxcar_Forcing_End_Time", &infectivity_boxcar_end_time, Boxcar_Infectivity_Forcing_End_Time_DESC_TEXT, 0.0f, 365.0f, 0.0f );
        }

        initConfig( "Animal_Reservoir_Type", animal_reservoir_type, config,
                    MetadataDescriptor::Enum( "animal_reservoir_type", Animal_Reservoir_Type_DESC_TEXT, MDD_ENUM_ARGS(AnimalReservoir) ) );

        if( animal_reservoir_type != AnimalReservoir::NO_ZOONOSIS || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Zoonosis_Rate", &zoonosis_rate, Zoonosis_Rate_DESC_TEXT, 0.0f, 1.0f, 0.0f );

            if( GET_CONFIGURABLE(SimulationConfig) &&
                ( GET_CONFIGURABLE(SimulationConfig)->heterogeneous_intranode_transmission_enabled || 
                  GET_CONFIGURABLE(SimulationConfig)->number_basestrains > 1 ||
                  GET_CONFIGURABLE(SimulationConfig)->number_substrains > 1 ) )
            {
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Zoonotic infection via an animal reservoir is not yet supported with heterogeneous intra-node transmission or multiple infection strains." );
            }
        }

        // TODO: there is conditionality in which of the following configurable parameters need to be read in based on the values of certain enums/booleans
        initConfigTypeMap( "Enable_Birth",                    &vital_birth,             Enable_Birth_DESC_TEXT,                   true,  "Enable_Vital_Dynamics"  );

        initConfigTypeMap( "Enable_Demographics_Other",       &demographics_other,      Enable_Demographics_Other_DESC_TEXT,      false );  // DJK*: Should be "Enable_Risk_Factor_Heterogeneity_At_Birth"
        initConfigTypeMap( "Enable_Demographics_Gender",      &demographics_gender,     Enable_Demographics_Gender_DESC_TEXT,     true  );  // DJK*: This needs to be configurable!

        initConfigTypeMap( "Enable_Maternal_Transmission",    &maternal_transmission,   Enable_Maternal_Transmission_DESC_TEXT,   false, "Enable_Birth" );
        initConfigTypeMap( "Enable_Demographics_Birth",       &demographics_birth,      Enable_Demographics_Birth_DESC_TEXT,      false, "Enable_Birth" );  // DJK*: Should be "Enable_Disease_Heterogeneity_At_Birth"
        initConfig( "Birth_Rate_Dependence", vital_birth_dependence, config, MetadataDescriptor::Enum(Birth_Rate_Dependence_DESC_TEXT, Birth_Rate_Dependence_DESC_TEXT, MDD_ENUM_ARGS(VitalBirthDependence)), "Enable_Birth" );

        initConfig( "Individual_Sampling_Type", ind_sampling_type, config, MetadataDescriptor::Enum("ind_sampling_type", Individual_Sampling_Type_DESC_TEXT, MDD_ENUM_ARGS(IndSamplingType)) );
        if( ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_IMMUNE_STATE )
        {
            initConfigTypeMap( "Relative_Sample_Rate_Immune",   &sample_rate_immune, Sample_Rate_Immune_DESC_TEXT,     0.001f, 1.0f, 0.1f );
            initConfigTypeMap( "Immune_Threshold_For_Downsampling",   &immune_threshold_for_downsampling, Immune_Threshold_For_Downsampling_DESC_TEXT,     0.0f, 1.0f, 0.0f );
        }

        if ( ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP || ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE )
        {
            initConfigTypeMap( "Sample_Rate_Birth",    &sample_rate_birth, Sample_Rate_Birth_DESC_TEXT,       0.0f, 1000.0f, 1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP || ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE");
            initConfigTypeMap( "Sample_Rate_0_18mo",   &sample_rate_0_18mo, Sample_Rate_0_18mo_DESC_TEXT,     0.0f, 1000.0f, 1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP || ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
            initConfigTypeMap( "Sample_Rate_18mo_4yr", &sample_rate_18mo_4yr, Sample_Rate_18mo_4yr_DESC_TEXT, 0.0f, 1000.0f, 1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP || ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
            initConfigTypeMap( "Sample_Rate_5_9",      &sample_rate_5_9, Sample_Rate_5_9_DESC_TEXT,           0.0f, 1000.0f, 1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP || ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
            initConfigTypeMap( "Sample_Rate_10_14",    &sample_rate_10_14, Sample_Rate_10_14_DESC_TEXT,       0.0f, 1000.0f, 1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP || ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
            initConfigTypeMap( "Sample_Rate_15_19",    &sample_rate_15_19, Sample_Rate_15_19_DESC_TEXT,       0.0f, 1000.0f, 1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP || ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
            initConfigTypeMap( "Sample_Rate_20_Plus",  &sample_rate_20_plus, Sample_Rate_20_Plus_DESC_TEXT,   0.0f, 1000.0f, 1.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_AGE_GROUP || ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );
        }

        initConfigTypeMap( "Base_Population_Scale_Factor",      &population_scaling_factor,  Base_Population_Scale_Factor_DESC_TEXT,      0.0f, FLT_MAX, 1.0f, "Population_Scale_Type", "FIXED_SCALING" );
        initConfigTypeMap( "Max_Node_Population_Samples",       &max_sampling_cell_pop,      Max_Node_Population_Samples_DESC_TEXT,       1.0f, FLT_MAX, 30.0f, "Individual_Sampling_Type", "ADAPTED_SAMPLING_BY_POPULATION_SIZE,ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE" );

        initConfig( "Population_Density_Infectivity_Correction", population_density_infectivity_correction, config, MetadataDescriptor::Enum("population_density_infectivity_correction", Population_Density_Infectivity_Correction_DESC_TEXT, MDD_ENUM_ARGS(PopulationDensityInfectivityCorrection)) ); // node only (move)

        if (population_density_infectivity_correction == PopulationDensityInfectivityCorrection::SATURATING_FUNCTION_OF_DENSITY)
        {
            initConfigTypeMap( "Population_Density_C50",        &population_density_c50,     Population_Density_C50_DESC_TEXT,            0.0f, FLT_MAX, 10.0f, "Population_Density_Infectivity_Correction", PopulationDensityInfectivityCorrection::pairs::lookup_key(PopulationDensityInfectivityCorrection::SATURATING_FUNCTION_OF_DENSITY) );
        }
        
        initConfigTypeMap( "x_Birth",                           &x_birth,                    x_Birth_DESC_TEXT,                           0.0f, FLT_MAX, 1.0f, "Enable_Birth" );
        
        initConfig( "Birth_Rate_Time_Dependence", vital_birth_time_dependence, config, MetadataDescriptor::Enum("vital_birth_time_dependence", Birth_Rate_Time_Dependence_DESC_TEXT, MDD_ENUM_ARGS(VitalBirthTimeDependence)), "Enable_Birth" ); // node only (move)
        if ((vital_birth_time_dependence == VitalBirthTimeDependence::SINUSOIDAL_FUNCTION_OF_TIME) || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Birth_Rate_Sinusoidal_Forcing_Amplitude", &birth_rate_sinusoidal_forcing_amplitude, Sinusoidal_Birth_Rate_Forcing_Amplitude_DESC_TEXT, 0.0f, 1.0f, 0.0f, "Birth_Rate_Time_Dependence", "SINUSOIDAL_FUNCTION_OF_TIME" );
            initConfigTypeMap( "Birth_Rate_Sinusoidal_Forcing_Phase", &birth_rate_sinusoidal_forcing_phase, Sinusoidal_Birth_Rate_Forcing_Phase_DESC_TEXT, 0.0f, 365.0f, 0.0f, "Birth_Rate_Time_Dependence", "SINUSOIDAL_FUNCTION_OF_TIME" );
        }

        
        initConfigTypeMap( "Birth_Rate_Boxcar_Forcing_Amplitude", &birth_rate_boxcar_forcing_amplitude, Boxcar_Birth_Rate_Forcing_Amplitude_DESC_TEXT, 0.0f, FLT_MAX, 0.0f, "Birth_Rate_Time_Dependence", "ANNUAL_BOXCAR_FUNCTION" );
        initConfigTypeMap( "Birth_Rate_Boxcar_Forcing_Start_Time", &birth_rate_boxcar_start_time, Boxcar_Birth_Rate_Forcing_Start_Time_DESC_TEXT, 0.0f, 365.0f, 0.0f, "Birth_Rate_Time_Dependence", "ANNUAL_BOXCAR_FUNCTION" );
        initConfigTypeMap( "Birth_Rate_Boxcar_Forcing_End_Time", &birth_rate_boxcar_end_time, Boxcar_Birth_Rate_Forcing_End_Time_DESC_TEXT, 0.0f, 365.0f, 0.0f, "Birth_Rate_Time_Dependence", "ANNUAL_BOXCAR_FUNCTION" );

        if( config && (*config).Exist( "Disable_IP_Whitelist" ) && (*config)["Disable_IP_Whitelist"].As<json::Number>() == 1 )
        {
            whitelist_enabled = false;
        }

        bool ret = JsonConfigurable::Configure( config );
        return ret;
    }

    void Node::Initialize()
    {
        setupEventContextHost();
        Configure( EnvPtr->Config );

        if(GET_CONFIGURABLE(SimulationConfig)->susceptibility_scaling == SusceptibilityScaling::LOG_LINEAR_FUNCTION_OF_TIME)
        {
            susceptibility_dynamic_scaling = 0.0f; // set susceptibility to zero so it may ramp up over time according to the scaling function
        }
    }

    void Node::setupEventContextHost()
    {
        event_context_host = _new_ NodeEventContextHost(this);
    }

    void Node::SetupMigration( IMigrationInfoFactory * migration_factory, 
                               MigrationStructure::Enum ms,
                               const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        migration_dist_type = DistributionFunction::NOT_INITIALIZED ;
        migration_dist1 = 0.0 ;
        migration_dist2 = 0.0 ;

        if( ms != MigrationStructure::NO_MIGRATION )
        {
            migration_info = migration_factory->CreateMigrationInfo( this, rNodeIdSuidMap );
            release_assert( migration_info != nullptr );

            if( migration_info->IsHeterogeneityEnabled() )
            {
                LOG_DEBUG( "Parsing MigrationHeterogeneityDistribution\n" );
                migration_dist_type = (DistributionFunction::Enum)(demographics["IndividualAttributes"]["MigrationHeterogeneityDistributionFlag"].AsInt());
                migration_dist1     = (float)                     (demographics["IndividualAttributes"]["MigrationHeterogeneityDistribution1"   ].AsDouble());
                migration_dist2     = (float)                     (demographics["IndividualAttributes"]["MigrationHeterogeneityDistribution2"   ].AsDouble());

                for( auto ind : this->individualHumans )
                {
                    // this is only done during initialization.  During the sim, configureAndAddNewIndividual() will set this
                    float temp_migration = (float)(Probability::getInstance()->fromDistribution(migration_dist_type, migration_dist1, migration_dist2, 1.0));
                    ind->SetMigrationModifier( temp_migration );
                }
            }                          
        }
    }

    void Node::SetMonteCarloParameters(float indsamplerate, int nummininf)
    {
        Ind_Sample_Rate         = indsamplerate;
    }

    void Node::checkIpKeyInWhitelist( const std::string& propertyKey, size_t numValues )
    {
        if( whitelist_enabled )
        {
            if( ipkeys_whitelist.count( propertyKey ) == 0 )
            {
                std::ostringstream msg;
                msg << "Individual Property key " << propertyKey << " found in demographics file. Use one of: ";
                for (auto& key : ipkeys_whitelist)
                {
                    msg << key << std::endl;
                }
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            if (((numValues > 5) && (propertyKey != "Geographic")) || (numValues > 125))
            {
                std::ostringstream msg;
                msg << "Too many values for Individual Property key " << propertyKey << ".  This key has " << numValues << " and the limit is 5, except for Geographic, which is 125." << std::endl;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
    }

    // Bad: 0.2, OK: 0.5, Good: 0.3
    // Distrib["QoC"][0.2] = "Bad";
    // Distrib["QoC"][0.7] = "Ok";
    // Distrib["QoC"][1.0] = "Good";
    tPropertiesDistrib Node::base_distribs;

    void Node::SetParameters(NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory)
    {
        // Parameters set from an input filestream
        // TODO: Jeff, this is a bit hack-y that I had to do this. is there a better way?
        NodeDemographics *demographics_temp = demographics_factory->CreateNodeDemographics(this);
        release_assert( demographics_temp );
        demographics = *(demographics_temp); // use copy constructor
        delete demographics_temp;
        externalId = demographics["NodeID"].AsInt();

        //////////////////////////////////////////////////////////////////////////////////////
        std::map< std::string, std::vector< std::string > > indivPropNameValuePairs;
        //std::map< std::string, std::map< std::string, float > > indivPropInits;
        LOG_DEBUG( "Looking for Individual_Properties in demographics.json file(s)\n" );
        if( demographics.Contains( IP_KEY ) )
        {
            LOG_INFO_F( "%d Individual_Properties found in demographics.json file(s)\n", demographics[IP_KEY].get_array().size() );
            // Check that we're not using more than 2 axes in whitelist mode
            if( demographics[IP_KEY].get_array().size() > 2 && 
                whitelist_enabled )
            {
                std::ostringstream msg;
                msg << "Too many Individual Property axes (" 
                    << demographics[IP_KEY].get_array().size()
                    << "). Max is 2."
                    << std::endl;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            json::Object tx_camp;
            tx_camp[ "Use_Defaults" ] = json::Number( 1 );
            tx_camp[ "Events" ] = json::Array();
            bool localized = false;
            for( int idx=0; idx<demographics[IP_KEY].get_array().size(); idx++ )
            {
                float maxProbBound = 0.0f;
                std::string propertyKey = demographics[IP_KEY][idx][IP_NAME_KEY].AsString();
                if( propertyKey == _age_bins_key )
                {
                    // iterate through array of age boundaries. For each one, create initialization transitions and calendars. 
                    JsonObjectDemog age_bin_tx( JsonObjectDemog::JSON_OBJECT_ARRAY );
                    const auto ageBinJsonArray = demographics[IP_KEY][idx][IP_AGE_BIN_KEY];
                    auto num_edges = demographics[IP_KEY][idx][IP_AGE_BIN_KEY].get_array().size();
                    float last_age_edge = -0.01f; // would use -1 but that's our max/"EOF" value.
                    for( int age_bin_idx=0; age_bin_idx<num_edges; age_bin_idx++ )
                    {
                        float age_edge = float(demographics[IP_KEY][idx][IP_AGE_BIN_KEY][age_bin_idx].AsDouble());
                        if( age_edge <= last_age_edge && age_edge != -1)
                        {
                            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "age_edge", age_edge, "last_age_edge", last_age_edge );
                        }
                        last_age_edge = age_edge;
                        if( age_bin_idx == 0 )
                        {
                           if( age_edge != 0 )
                           {
                               std::ostringstream errMsg;
                               errMsg << "First age bin value must be 0 (not " << age_edge << ").";
                               throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
                           }
                           else
                           {
                               continue;
                           }
                        }
                        else if( age_bin_idx == num_edges-1 )
                        {
                           if( age_edge != -1 )
                           {
                               std::ostringstream msg;
                               msg << "Value of final edge in "
                                   << IP_AGE_BIN_KEY
                                   << " array must be -1.";
                               throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                           }
                        }
                        // Put people in correct bucket at first
                        float min_age = float(ageBinJsonArray[age_bin_idx-1].AsDouble());
                        float max_age = age_edge;
                        if( max_age == -1 )
                        {
                            max_age = MAX_HUMAN_AGE;
                        }
                        JsonObjectDemog initTx( JsonObjectDemog::JSON_OBJECT_OBJECT );
                        initTx.Add( "From", "NULL" );
                        initTx.Add( "To", getAgeBinPropertyNameFromIndex( ageBinJsonArray, age_bin_idx) );
                        // TBD: following line generates huge warning for MSVC
                        distribs[ _age_bins_key ].insert( make_pair( 0.0f, getAgeBinPropertyNameFromIndex( ageBinJsonArray, age_bin_idx) ) ); // for PropertyReport
                        initTx.Add( "Type", "At_Timestep" );

                        JsonObjectDemog startStopObj( JsonObjectDemog::JSON_OBJECT_OBJECT );
                        startStopObj.Add( "Start", 0.0 );
                        startStopObj.Add( "Duration", -1.0 );
                        initTx.Add( IP_WHEN_KEY, startStopObj );

                        //initTx.initTx.Add( "From", "NULL" );
                        //initTx.initTx.Add("To", getAgeBinPropertyNameFromIndex( ageBinJsonArray, age_bin_idx) );
                        // TBD: following line generates huge warning for MSVC
                        //distribs[ _age_bins_key ].insert( make_pair( 0.0f, getAgeBinPropertyNameFromIndex( ageBinJsonArray, age_bin_idx) ) ); // for PropertyReport
                        initTx.Add( "Type", "At-Time" );
                        initTx.Add( "When", 0.0 );
                        initTx.Add( "Coverage", 1.0 );

                        JsonObjectDemog minmaxObj( JsonObjectDemog::JSON_OBJECT_OBJECT );
                        minmaxObj.Add( "Min", min_age );
                        minmaxObj.Add( "Max", max_age );
                        initTx.Add( "Age_Restrictions", minmaxObj );
                        initTx.Add( IP_AGE_KEY, minmaxObj );
                        
                        initTx.Add( IP_PROBABILITY_KEY, 1.0 );
                        initTx.Add( IP_REVERSION_KEY, 0.0 );
                        age_bin_tx.PushBack( initTx );

                        // Give people calendars to move to next bucket.
                        // All folks in age_bins<max-1 need calendars for all subsequent age_bins!
                        if( age_bin_idx > 1 )
                        {
                            JsonObjectDemog bdayTx( JsonObjectDemog::JSON_OBJECT_OBJECT );
                            bdayTx.Add( "From", getAgeBinPropertyNameFromIndex( ageBinJsonArray, age_bin_idx-1) );
                            bdayTx.Add( "To", getAgeBinPropertyNameFromIndex( ageBinJsonArray, age_bin_idx) );
                            //distribs[ _age_bins_key ].insert( make_pair( 0.0f, getAgeBinPropertyNameFromIndex( ageBinJsonArray, age_bin_idx) ) ); // for PropertyReport

                            JsonObjectDemog binStartStopObj( JsonObjectDemog::JSON_OBJECT_OBJECT );
                            binStartStopObj.Add( "Start", 1.0 );
                            binStartStopObj.Add( "Duration", -1.0 );
                            bdayTx.Add( IP_WHEN_KEY, binStartStopObj );

                            bdayTx.Add( "Type", "At_Age" );
                            bdayTx.Add( "Age_In_Years", min_age );
                            bdayTx.Add( "Coverage", 1.0 );

                            JsonObjectDemog binMinMaxObj( JsonObjectDemog::JSON_OBJECT_OBJECT );
                            binMinMaxObj.Add( "Min", 0.0 );
                            binMinMaxObj.Add( "Max", min_age );
                            bdayTx.Add( IP_AGE_KEY, binMinMaxObj );
                            
                            bdayTx.Add( IP_PROBABILITY_KEY, 1.0 );
                            bdayTx.Add( IP_REVERSION_KEY, 0.0 );
                            age_bin_tx.PushBack( bdayTx );
                        }
                    }

                    // convert Transitions to transitions.json
                    // Create NodeDemographics object from json_spirit so that we can
                    // call convertTransitions which assumes NodeDemographics input.
                    auto tx_full_string_table = new map<string, string>();
                    NodeDemographics manual_transitions_node_demographics( age_bin_tx, tx_full_string_table, (INodeContext*) this, externalId, "transitions", "" );
                    convertTransitions( manual_transitions_node_demographics, tx_camp, propertyKey );
                }
                else
                {
                    if( !demographics[IP_KEY][idx].Contains( IP_VALUES_KEY ) )
                    {
                        std::ostringstream badMap;
                        badMap << "demographics[" << IP_KEY << "][" << idx << "]";
                        throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, badMap.str().c_str(), IP_VALUES_KEY );
                    }

                    auto num_values = demographics[IP_KEY][idx][IP_VALUES_KEY].get_array().size();
                    checkIpKeyInWhitelist( propertyKey, num_values );
                    for( int val_idx=0; val_idx<num_values; val_idx++ )
                    {
                        std::string value = demographics[IP_KEY][idx][IP_VALUES_KEY][val_idx].AsString();
                        if( std::find( indivPropNameValuePairs[ propertyKey ].begin(), indivPropNameValuePairs[ propertyKey ].end(), value ) != indivPropNameValuePairs[ propertyKey ].end() )
                        {
                            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ( std::string( "Duplicate Value found: " ) + value ).c_str() );
                        }
                        indivPropNameValuePairs[ propertyKey ].push_back( value );
                    }
                    if( demographics[IP_KEY][idx][IP_INIT_KEY].get_array().size() != demographics[IP_KEY][idx][IP_VALUES_KEY].get_array().size() )
                    {
                        std::ostringstream msg;
                        msg << "Number of Values ("
                            << demographics[IP_KEY][idx][IP_VALUES_KEY].get_array().size() 
                            << ") needs to be the same as number of "
                            << IP_INIT_KEY
                            << "'s ("
                            << demographics[IP_KEY][idx][IP_INIT_KEY].get_array().size()
                            << ")."
                            << std::endl;
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                    }
                    const char* use_this_init_key = IP_KEY;

                    std::string ip_key_localized( IP_KEY );
                    ip_key_localized += "::Localized";
                    const char* localized_key = ip_key_localized.c_str();
                    // const char* localized_key = ( std::string( IP_KEY ) + "::Localized" ).c_str();

                    if( demographics.Contains( localized_key ) )
                    {
                        use_this_init_key = localized_key;
                        localized = true;
                    }
                    auto init_values = demographics[use_this_init_key][idx][IP_INIT_KEY];
                    for( int val_idx=0; val_idx<init_values.get_array().size(); val_idx++ )
                    {
                        try
                        {
                            ProbabilityNumber init_value = init_values[val_idx].AsDouble();

                            const std::string& value = indivPropNameValuePairs[ propertyKey ][ val_idx ];
                            maxProbBound += init_value;
                            distribs[ propertyKey ].insert( make_pair( maxProbBound, value ) );
                            LOG_DEBUG_F( "Found IP distribs[ %s ][ %f ] = %s\n", propertyKey.c_str(), maxProbBound, value.c_str() );
                        }
                        catch( OutOfRangeException ex )
                        {
                            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, IP_INIT_KEY, demographics[IP_KEY][idx][IP_INIT_KEY][val_idx].AsDouble(), 0 );
                        }
                    }
                    if( maxProbBound < 0.99999 || maxProbBound > 1.000001 )
                    {
                        std::ostringstream msg;
                        msg << "Bin probabilities in "
                            << IP_INIT_KEY
                            << " section for property "
                            << propertyKey
                            << " must add up to 1.0. Instead came to "
                            << maxProbBound
                            << "."
                            << std::endl;
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                    }
                    // convert Transitions to ip_transitions_campaign.json
                    convertTransitions( demographics[ IP_KEY ][ idx ][ "Transitions" ], tx_camp, propertyKey );
                }
            }

            // This is node-level code but we only want to open this file once regardless how many nodes we have.
            // An implicit assumption is that all ranks have at least one node.
            static bool doOnce = false;
            if( doOnce==false )
            {
                std::string transitions_file_path = FileSystem::Concat( Environment::getInstance()->OutputPath, std::string(Node::transitions_dot_json_filename) );
                // Rank 0 creates the transitions json file
                if( EnvPtr->MPI.Rank == 0 )
                {
                    LOG_DEBUG_F( "Creating %s file.\n", transitions_file_path.c_str() );
                    std::ofstream tx_camp_json( transitions_file_path.c_str() );
                    json::Writer::Write( tx_camp, tx_camp_json );
                }

                // Everybody stops here for a sync-up after rank 0 writes transitions.json
                EnvPtr->MPI.p_idm_mpi->Barrier();

                doOnce = true;
            }

            if( base_distribs.size() == 0 && localized == false ) // set base_distribs once we have parsed a node that has no localizations
            {
                LOG_DEBUG_F( "Using initial distributions for node %lu as base for all subsequent nodes.\n", GetSuid().data );
                base_distribs = distribs;
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////

        birthrate = float(demographics["NodeAttributes"]["BirthRate"].AsDouble());
        
        release_assert(params());
        if (demographics_other) 
        {
            Above_Poverty = float(demographics["NodeAttributes"]["AbovePoverty"].AsDouble());
            urban = (demographics["NodeAttributes"]["Urban"].AsInt() != 0);

            if (GET_CONFIGURABLE(SimulationConfig)->coinfection_incidence == true)
            {
                demographic_distributions[NodeDemographicsDistribution::HIVCoinfectionDistribution] = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["HIVCoinfectionDistribution"], "gender", "time", "age");
                demographic_distributions[NodeDemographicsDistribution::HIVMortalityDistribution]   = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["HIVMortalityDistribution"], "gender", "time", "age");
            }
        }

        VitalDeathDependence::Enum vital_death_dependence = GET_CONFIGURABLE(SimulationConfig)->vital_death_dependence;
        if( vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_AGE_AND_GENDER )
        {
            LOG_DEBUG( "Parsing IndividualAttributes->MortalityDistribution tag in node demographics file.\n" );
            demographic_distributions[NodeDemographicsDistribution::MortalityDistribution] = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["MortalityDistribution"], "gender", "age");
        }
        else if( vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_YEAR_AND_AGE_FOR_EACH_GENDER )
        {
            LOG_DEBUG( "Parsing IndividualAttributes->MortalityDistributionMale and IndividualAttributes->MortalityDistributionFemale tags in node demographics file.\n" );
            demographic_distributions[NodeDemographicsDistribution::MortalityDistributionMale] = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["MortalityDistributionMale"], "age", "year");
            demographic_distributions[NodeDemographicsDistribution::MortalityDistributionFemale] = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["MortalityDistributionFemale"], "age", "year");
        }

        if (params()->immunity_initialization_distribution_type == DistributionType::DISTRIBUTION_COMPLEX)
        {
            LoadImmunityDemographicsDistribution();
        }

        if (vital_birth_dependence != VitalBirthDependence::FIXED_BIRTH_RATE)// births per cell per day is population dependent
        {
            // If individual pregnancies will begin based on age-dependent fertility rates, create the relevant distribution here:
            if(vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE) 
            {
                LOG_DEBUG( "Parsing IndividualAttributes->FertilityDistribution tag in node demographics file.\n" );
                demographic_distributions[NodeDemographicsDistribution::FertilityDistribution] = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["FertilityDistribution"], "urban", "age");
            }
            else if(vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR)
            {
                LOG_DEBUG( "Parsing IndividualAttributes->FertilityDistribution tag in node demographics file.\n" );
                demographic_distributions[NodeDemographicsDistribution::FertilityDistribution] = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["FertilityDistribution"], "age", "year");
            }

            if (birthrate > BIRTHRATE_SANITY_VALUE)
            {
                // report error message to error file and error iostream
                // ERROR: ("Check birthrate/vital_birth_dependence mismatch in Node::SetParameters()\n");
                //throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__ );

                // Error handling in execution without instant termination
                // still allow simulation to run, but set birthrate depending on value of vital_birth_dependence
                if (vital_birth_dependence == VitalBirthDependence::POPULATION_DEP_RATE)
                {
                    birthrate = float(TWO_PERCENT_PER_YEAR); // TBD: literal should be defined as float
                }
                else if ( vital_birth_dependence == VitalBirthDependence::DEMOGRAPHIC_DEP_RATE   ||
                    vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES ||
                    vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE ||
                    vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR )
                {
                    birthrate = float(INDIVIDUAL_BIRTHRATE); // TBD: literal should be defined as float // DJK: why is this even needed for age-specific fertility?
                }
                else
                {
                    birthrate = float(FALLBACK_BIRTHRATE);
                }
            }
        }
#ifndef DISABLE_CLIMATE
        //  Set climate
        //if (GET_CONFIGURABLE(SimulationConfig)->climate_structure != ClimateStructure::CLIMATE_OFF)   // DJK: Why commented?
        {
            LOG_DEBUG( "Parsing NodeAttributes->Altitude tag in node demographics file.\n" );
            float altitude = float(demographics["NodeAttributes"]["Altitude"].AsDouble());
            localWeather = climate_factory->CreateClimate(this, altitude, GetLatitudeDegrees() );
        }
#endif
        // Scale by zoonosis risk in demographics layer
        if( animal_reservoir_type == AnimalReservoir::ZOONOSIS_FROM_DEMOGRAPHICS )
        {
            LOG_DEBUG( "Parsing NodeAttributes->Zoonosis tag in node demographics file.\n" );
            zoonosis_rate *= demographics["NodeAttributes"]["Zoonosis"].AsDouble();
        }
        SetupIntranodeTransmission();
    }

    void Node::LoadImmunityDemographicsDistribution()
    {
        // For GENERIC_SIM (and related human-immune-model classes), "ImmunityDistribution" is an age-specific probability of being immune
        // For derived classes, similar age-dependent distributions relate specifically to vaccine dose history (POLIO_SIM) and MSP/PfEMP1 antibodies (MALARIA_SIM)
        LOG_DEBUG( "Parsing IndividualAttributes->ImmunityDistribution tag in node demographics file.\n" );
        demographic_distributions[NodeDemographicsDistribution::ImmunityDistribution] = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["ImmunityDistribution"], "age");
    }

    void Node::SetupIntranodeTransmission()
    {
        LOG_DEBUG_F( "%s\n", __FUNCTION__ );
        transmissionGroups = TransmissionGroupsFactory::CreateNodeGroups(TransmissionGroupType::SimpleGroups);
        RouteToContagionDecayMap_t decayMap;
        if( demographics.Contains( IP_KEY ) && params()->heterogeneous_intranode_transmission_enabled)
        {
            ValidateIntranodeTransmissionConfiguration();

            const NodeDemographics& properties = demographics[IP_KEY];
            for (int iProperty = 0; iProperty < properties.size(); iProperty++)
            {
                const NodeDemographics& property = properties[iProperty];
                if (property.Contains(TRANSMISSION_MATRIX_KEY))
                {
                    const NodeDemographics& transmissionMatrix = property[ TRANSMISSION_MATRIX_KEY ];
                    PropertyValueList_t valueList;
                    const NodeDemographics& scalingMatrixRows = transmissionMatrix[TRANSMISSION_DATA_KEY];
                    ScalingMatrix_t scalingMatrix;

                    string routeName = transmissionMatrix.Contains( ROUTE_KEY ) ? transmissionMatrix[ ROUTE_KEY ].AsString() : "contact";
                    std::transform(routeName.begin(), routeName.end(), routeName.begin(), ::tolower);
                    string propertyName = property[IP_NAME_KEY].AsString();

                    if (routeName != "contact")
                    {
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, std::string( "Found route " + routeName + ". For generic sims, routes other than 'contact' are not supported, use Environmental sims for 'environmental' decay.").c_str());
                    }
                    else
                    {
                        if (decayMap.find(routeName)==decayMap.end())
                        {
                            LOG_DEBUG_F("HINT: Adding route %s.\n", routeName.c_str());
                            decayMap[routeName] = 1.0f;
                            routes.push_back(routeName);
                        }
                    }

                    if( propertyName == _age_bins_key )
                    {
                        int valueCount = distribs[ _age_bins_key ].size();
                        int counter = 0;
                        for( const auto& entry : distribs[ _age_bins_key ])
                        {
                            valueList.push_back( entry.second );
                            MatrixRow_t matrixRow;
                            const NodeDemographics& scalingMatrixRow = scalingMatrixRows[counter++];
    
                            for (int iSink = 0; iSink < valueCount; iSink++) 
                            {
                                 matrixRow.push_back(float(scalingMatrixRow[iSink].AsDouble()));
                            }
                            scalingMatrix.push_back(matrixRow);
                        }
                    }
                    else
                    {
                        const NodeDemographics& propertyValues = property[ IP_VALUES_KEY ];
                        int valueCount = propertyValues.size();
                        for (int iValue = 0; iValue < valueCount; iValue++)
                        {
                            valueList.push_back(propertyValues[iValue].AsString());
                            MatrixRow_t matrixRow;
                            const NodeDemographics& scalingMatrixRow = scalingMatrixRows[iValue];
    
                            for (int iSink = 0; iSink < valueCount; iSink++) 
                            {
                                 matrixRow.push_back(float(scalingMatrixRow[iSink].AsDouble()));
                            }
                            scalingMatrix.push_back(matrixRow);
                        }
                    }

                    LOG_DEBUG_F("adding property [%s]:%s\n", propertyName.c_str(), routeName.c_str());
                    transmissionGroups->AddProperty(propertyName, valueList, scalingMatrix, routeName);
                }
                else //HINT is enabled, but no transmission matrix is detected
                {
                    string default_route("contact");
                    float default_rate = 1.0f;
                    if (decayMap.find(default_route)==decayMap.end())
                    {
                        LOG_DEBUG("HINT on with no transmission matrix: Adding route 'contact'.\n");
                        decayMap[default_route] = default_rate;
                        routes.push_back(default_route);
                    }
                }
            }

        }
        else //HINT is not enabled
        {
            LOG_DEBUG("Non-HINT: Adding route 'contact'.\n");
            decayMap[string("contact")] = 1.0f;
            routes.push_back(string("contact"));
        }

        transmissionGroups->Build(decayMap, 1, 1);
    }

    void Node::GetGroupMembershipForIndividual(const RouteList_t& route, tProperties* properties, TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        LOG_DEBUG_F( "Calling GetGroupMembershipForProperties\n" );
        transmissionGroups->GetGroupMembershipForProperties( route, properties, transmissionGroupMembership );
    }

    float Node::GetTotalContagion(const TransmissionGroupMembership_t* membership)
    {
        return transmissionGroups->GetTotalContagion(membership);
    }

    const RouteList_t& Node::GetTransmissionRoutes() const
    {
        return routes;
    }

    void Node::UpdateTransmissionGroupPopulation(const TransmissionGroupMembership_t* membership, float size_changes,float mc_weight)
    {
        transmissionGroups->UpdatePopulationSize(membership, size_changes, mc_weight);
    }

    void Node::ExposeIndividual(IInfectable* candidate, const TransmissionGroupMembership_t* individual, float dt)
    {
        transmissionGroups->ExposeToContagion(candidate, individual, dt);
    }

    void Node::DepositFromIndividual(StrainIdentity* strain_IDs, float contagion_quantity, const TransmissionGroupMembership_t* individual)
    {
        LOG_DEBUG_F("deposit from individual: antigen index =%d, substain index = %d, quantity = %f\n", strain_IDs->GetAntigenID(), strain_IDs->GetGeneticID(), contagion_quantity);
        transmissionGroups->DepositContagion(strain_IDs, contagion_quantity, individual);
    }
    
    act_prob_vec_t Node::DiscreteGetTotalContagion(const TransmissionGroupMembership_t* membership)
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, 
            "The use of DiscreteGetTotalContagion is not supported in \"GENERIC_SIM\".  \
             To use discrete transmission, please use a simulation type derived from either \
             \"STI_SIM\" or \"HIV_SIM\"." );
    }

    //------------------------------------------------------------------
    //   Every timestep Update() methods
    //------------------------------------------------------------------

    void Node::SetWaitingForFamilyTrip( suids::suid migrationDestination, 
                                        MigrationType::Enum migrationType, 
                                        float timeUntilTrip, 
                                        float timeAtDestination,
                                        bool isDestinationNewHome )
    {
        family_waiting_to_migrate      = true;
        family_migration_destination   = migrationDestination;
        family_migration_type          = migrationType;
        family_time_until_trip         = timeUntilTrip;
        family_time_at_destination     = timeAtDestination;
        family_is_destination_new_home = isDestinationNewHome;
    }

    void Node::ManageFamilyTrip( float currentTime, float dt )
    {
        if( family_waiting_to_migrate )
        {
            bool leave_on_trip = IsEveryoneHome() ;
            for (auto individual : individualHumans)
            {
                if( home_individual_ids.count( individual->GetSuid().data ) > 0 )
                {
                    if( leave_on_trip )
                    {
                        individual->SetGoingOnFamilyTrip( family_migration_destination, 
                                                          family_migration_type, 
                                                          family_time_until_trip, 
                                                          family_time_at_destination,
                                                          family_is_destination_new_home );
                    }
                    else
                    {
                        individual->SetWaitingToGoOnFamilyTrip();
                    }
                }
            }
            if( leave_on_trip )
            {
                family_waiting_to_migrate      = false ;
                family_migration_destination   = suids::nil_suid();
                family_migration_type          = MigrationType::NO_MIGRATION;
                family_time_until_trip         = 0.0f;
                family_time_at_destination     = 0.0f ;
                family_is_destination_new_home = false;
            }
            else
            {
                family_time_until_trip -= dt ;
            }
        }
    }

    void Node::Update(float dt)
    {

        // Update weather
        //if(GET_CONFIGURABLE(SimulationConfig)->climate_structure != ClimateStructure::CLIMATE_OFF)
        if(localWeather)
        {
            localWeather->UpdateWeather(GetTime().time, dt);
        }

        // Update node-level interventions
        if (params()->interventions) 
        {
            release_assert(event_context_host);
            event_context_host->UpdateInterventions(dt); // update refactored node-owned node-targeted interventions

            // -------------------------------------------------------------------------
            // --- I'm putting this after updating the interventions because if one was
            // --- supposed to expire this timestep, then this event should not fire it.
            // -------------------------------------------------------------------------
            for( auto event_name : events_from_other_nodes )
            {
                for (auto individual : individualHumans)
                {
                    event_context_host->TriggerNodeEventObserversByString( individual->GetEventContext(), event_name );
                }
            }
        }

        ManageFamilyTrip( GetTime().time, dt );

        //-------- Accumulate infectivity and reporting counters ---------

        resetNodeStateCounters();

        // Update the likelihood of an individual becoming infected.
        // This is based on the current infectiousness at the start of the timestep of all individuals present at the start of the timestep
        updateInfectivity(dt);

        for (auto individual : individualHumans)
        {
            individual->Update(GetTime().time, dt);

            // JPS: Should we do this later, after updateVitalDynamics() instead?  
            //      That way we could track births in the report class instead of having to do it in Node...
            for (auto report : parent->GetReportsNeedingIndividualData())
            {
                report->LogIndividualData(individual);
            }

            updateNodeStateCounters(individual);
        }

        finalizeNodeStateCounters();

        //----------------------------------------------------------------

        // Vital dynamics for this time step at community level (handles mainly births)
        if (params()->vital_dynamics)
        {
            updateVitalDynamics(dt);
        }

        if (ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_IMMUNE_STATE)
        {
            LOG_DEBUG_F( "Check whether any individuals need to be down-sampled based on immunity.\n" );
            for (auto individual : individualHumans) 
            {
                float current_mc_weight = float(individual->GetMonteCarloWeight());
                if (current_mc_weight == 1.0f/Ind_Sample_Rate)  //KM: In immune state downsampling, there is only the regular sampling and downsampled; don't need to go through the logic if we've already been downsampled.
                {
                   float desired_mc_weight = 1.0f/float(adjustSamplingRateByImmuneState(Ind_Sample_Rate, ( individual->GetAcquisitionImmunity() < immune_threshold_for_downsampling ) && !individual->IsInfected() ));
                   if (desired_mc_weight > current_mc_weight)
                   {
                       LOG_DEBUG_F("MC Sampling: acq. immunity = %f, is infected = %i, current MC weight = %f, desired MC weight = %f\n", individual->GetAcquisitionImmunity(), individual->IsInfected(), current_mc_weight,desired_mc_weight);
                       individual->UpdateMCSamplingRate(desired_mc_weight);
                       LOG_DEBUG_F("MC Weight now %f, state change = %i\n", float(individual->GetMonteCarloWeight()), (int)individual->GetStateChange());
                   }
                   /*else if (desired_mc_weight < current_mc_weight)
                   {
                       //KM: Placeholder to allow a "clone individual and increase sampling rate" functionality if desired
                   }*/
                }
            }
        }

        // If individual has migrated or died -- HAPPENS AT THE END OF TIME STEP -- he/she still contributes to the infectivity
        for( int iHuman = 0 ; iHuman < individualHumans.size() ; /* control in loop */ )
        {
            IIndividualHuman* individual = individualHumans[iHuman];
            release_assert( individual );

            auto state_change = individual->GetStateChange();
            if( individual->IsDead() )
            {
                if (individual->GetStateChange() == HumanStateChange::KilledByInfection)
                    Disease_Deaths += (float)individual->GetMonteCarloWeight();

                individual->UpdateGroupPopulation(-1.0f);
                RemoveHuman( iHuman );

                // ---------------------------------------
                // --- We want individuals to die at home
                // ---------------------------------------
                if( individual->AtHome() )
                {
                    home_individual_ids.erase( individual->GetSuid().data ); // if this person doesn't call this home, then nothing happens

                    delete individual;
                    individual = NULL;
                }
                else
                {
                    // individual must go home to officially die
                    individual->GoHome();
                    processEmigratingIndividual(individual);
                }
            }
            else if (individual->IsMigrating())
            {
                // don't remove from home_individual_ids because they are just migrating

                RemoveHuman( iHuman );

                // subtract individual from group population(s)
                individual->UpdateGroupPopulation(-1.0f);
                processEmigratingIndividual(individual);
            }
            else
            {
                ++iHuman;
            }
        }

        if(GET_CONFIGURABLE(SimulationConfig)->susceptibility_scaling == SusceptibilityScaling::LOG_LINEAR_FUNCTION_OF_TIME)
        {
            susceptibility_dynamic_scaling += dt * GET_CONFIGURABLE(SimulationConfig)->susceptibility_scaling_rate;

            if(susceptibility_dynamic_scaling > 1.0f)
                susceptibility_dynamic_scaling = 1.0f; // once susceptibility reaches its nominal value it stays there
        }

        // Increment simulation time counter
    }

    void Node::updateInfectivity(float dt)
    {
        // Decay contagion from previous time step(s)
        //decayContagion(dt);

        // Process population to update who is infectious, etc...
        updatePopulationStatistics(dt);
        LOG_DEBUG_F("Statistical population of %d at Node ID = %d.\n", GetStatPop(), GetSuid().data);

        if ( statPop <=0 )
        {
            infectionrate = 0;
            LOG_WARN_F("No individuals at Node ID = %d.  infectionrate = %f\n", GetSuid().data, infectionrate);
            return;
        }

        //single route implementation
        float infectivity_correction = 1.0;      // Density-independent transmission
        infectionrate = mInfectivity;            // For reporting only.  Accumulation of infectivity and normalization (e.g by population size) are done on socialNetwork.
        LOG_DEBUG_F("[updateInfectivity] starting infectionrate = %f\n", infectionrate);

        // Add-in population density correction
        if (population_density_infectivity_correction == PopulationDensityInfectivityCorrection::SATURATING_FUNCTION_OF_DENSITY)
        {
            infectivity_correction *= getDensityContactScaling();
        }

        infectionrate *= infectivity_correction / statPop; // Normalization by StatPop and density correction, does not include additional factors below for seasonality
        LOG_DEBUG_F("[updateInfectivity] corrected infectionrate = %f\n", infectionrate);

        // Add in seasonality
        if (infectivity_scaling == InfectivityScaling::FUNCTION_OF_TIME_AND_LATITUDE)
        {
            infectivity_correction *= getSeasonalInfectivityCorrection();
        }
        else if (infectivity_scaling == InfectivityScaling::FUNCTION_OF_CLIMATE)
        {
            infectivity_correction *= getClimateInfectivityCorrection();
        }
        else if(infectivity_scaling == InfectivityScaling::SINUSOIDAL_FUNCTION_OF_TIME)
        {
            infectivity_correction *= getSinusoidalCorrection(infectivity_sinusoidal_forcing_amplitude, 
                                                              infectivity_sinusoidal_forcing_phase);	
        }
        else if(infectivity_scaling == InfectivityScaling::ANNUAL_BOXCAR_FUNCTION)
        {
            infectivity_correction *= getBoxcarCorrection(infectivity_boxcar_forcing_amplitude,
                                                          infectivity_boxcar_start_time,
                                                          infectivity_boxcar_end_time);	
        }
        // If there is an animal reservoir, deposit additional contagion
        if( animal_reservoir_type != AnimalReservoir::NO_ZOONOSIS )
        {
            // Default to strain with (antigen, strain) = (0,0)
            StrainIdentity strainId = StrainIdentity();

            // The per-individual infection rate will be later normalized by weighted population in transmissionGroups
            // The "zoonosis_rate" parameter is per individual, so scale by "statPop" before depositing contagion.
            // N.B. the "Daily (Human) Infection Rate" reporting channel does not include this effect or the InfectivityScaling above

            // Assume no heterogeneous transmission
            TransmissionGroupMembership_t defaultGroupMembership;
            defaultGroupMembership[0] = 0;
            transmissionGroups->DepositContagion(&strainId, zoonosis_rate * statPop, &defaultGroupMembership);

        }        
        transmissionGroups->EndUpdate(infectivity_correction);
        LOG_DEBUG_F("[updateInfectivity] final infectionrate = %f\n", infectionrate);
    }

    void Node::accumulateIndividualPopulationStatistics(float dt, IIndividualHuman* individual)
    {
        float mc_weight = float(individual->GetMonteCarloWeight());

        individual->UpdateInfectiousness(dt);
        float infectiousness = mc_weight * individual->GetInfectiousness();
        if( infectiousness > 0 )
        {
            LOG_DEBUG_F( "infectiousness = %f\n", infectiousness );
        }

        // These are zeroed out in ResetNodeStateCounters before being accumulated each time step
        statPop          += mc_weight;
        Possible_Mothers += long(individual->IsPossibleMother() ? mc_weight : 0);
        mInfectivity     += infectiousness;
        Infected         += individual->IsInfected() ? mc_weight : 0;
    }

    void Node::updatePopulationStatistics(float dt)
    {
        for (auto individual : individualHumans)
        {
            // This function is modified in derived classes to accumulate
            // disease-specific individual properties
            accumulateIndividualPopulationStatistics(dt, individual);
        }
    }

    float Node::getDensityContactScaling()
    {
        float localdensity      = 0;
        float densitycorrection = 1.00;

        if (params()->lloffset > 0) // check to make sure area will be nonzero
        {
            // calculate area of cell in km^2 from the lloffset, which is half the cell width in degrees
            // under the sphere coordinate, dA = R^2*sin(theta) d theta d phi
            // where theta is 90 degrees-latitude, phi is latitude, both in radians
            // note: under spherical coordinate, theta is 90 degree - latitude
            // therefore, integrate between theta1, theta2 and phi1, phi2, A = R^2 * (phi2-phi1) * cos(theta1)-cos(theta2)

            // in degrees,
            // phi2-phi1 = 2 *lloffset
            // theta1 = 90 - latitude - lloffset
            // theta2 = 90 - latitude + lloffset

            // Pi/180 will convert degree to radian
            float lat_deg = GetLatitudeDegrees() ;
            float lat_rad1 = float(( 90.0 - lat_deg - params()->lloffset ) * PI / 180.0);
            float lat_rad2 = float(( 90.0 - lat_deg + params()->lloffset ) * PI / 180.0);

            float cellarea = float(EARTH_RADIUS_KM * EARTH_RADIUS_KM * (cos(lat_rad1) - cos(lat_rad2)) * params()->lloffset * 2.0 * (PI / 180.0));

            localdensity = statPop / cellarea;
            LOG_DEBUG_F("[getDensityContactScaling] cellarea = %f, localdensity = %f\n", cellarea, localdensity);
        }

        if (population_density_c50 > 0)
        {
            densitycorrection = EXPCDF(-localdensity / population_density_c50);
        }
        else
        {
            densitycorrection = 1.0;
        }

        LOG_DEBUG_F("[getDensityContactScaling] densitycorrection = %f\n", densitycorrection);
        return densitycorrection;
    }

    float Node::getSeasonalInfectivityCorrection()
    {
        // This FUNCTION_OF_TIME_AND_LATITUDE correction varies infectivity from its full value to half its value as a function of latitude and time of year with a one year period
        // to approximate seasonal forcing for different diseases which have a peak season from fall to spring
        // When infectivity is at its full value in the northern hemisphere, it is at its lowest value in the southern hemisphere
        // The phase is chosen so that in spring and fall, the infectivities in northern and southern latitude are approximately equal.
        // the latitude offset moves the center of the peak infectivity cosine from 23.5 N latitude to 23.5 S latitude

        float lat_deg = GetLatitudeDegrees();
        float correction = 1.0 / 4 * (3.0 - cos(2 * 2 * PI / 360 * (lat_deg - 23.5 * sin((GetTime().time - 100) / DAYSPERYEAR * 2.0 * PI))));
        LOG_DEBUG_F( "Infectivity scale factor = %f at latitude = %f and simulation time = %f.\n", correction, lat_deg, (float) GetTime().time );

        return correction;
    }

    float Node::getClimateInfectivityCorrection() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, 
            "The use of \"Infectivity_Scale_Type\" : \"FUNCTION_OF_CLIMATE\" is not supported in \"GENERIC_SIM\".  \
             To have an explicit dependence of infectivity on climate, please use a simulation type derived from either \
             \"AIRBORNE_SIM\" or \"ENVIRONMENTAL_SIM\"." );
    }

    void Node::updateVitalDynamics(float dt)
    {
        long int newborns    = 0;
        float step_birthrate = birthrate * dt * x_birth;

        if (!vital_birth) //  Births
        {
            return;
        }

        switch(vital_birth_time_dependence)
        { 
            case VitalBirthTimeDependence::NONE:
            break; 

            case VitalBirthTimeDependence::SINUSOIDAL_FUNCTION_OF_TIME:
            step_birthrate *= getSinusoidalCorrection(birth_rate_sinusoidal_forcing_amplitude, 
                                                      birth_rate_sinusoidal_forcing_phase);
            break; 

            case VitalBirthTimeDependence::ANNUAL_BOXCAR_FUNCTION:
            step_birthrate *= getBoxcarCorrection(birth_rate_boxcar_forcing_amplitude,    
                                                  birth_rate_boxcar_start_time,
                                                  birth_rate_boxcar_end_time);
            break;
        }

        switch (vital_birth_dependence)
        {
            case VitalBirthDependence::FIXED_BIRTH_RATE:
            //  Calculate births for this time step from constant rate and add them
            newborns = long(randgen->Poisson(step_birthrate));
            populateNewIndividualsByBirth(newborns);
            break;

            case VitalBirthDependence::POPULATION_DEP_RATE:
            //  Birthrate dependent on current population, determined by census
            if (ind_sampling_type) {  newborns = long(randgen->Poisson(step_birthrate * statPop)); }        // KTO !TRACK_ALL?  is this right?
            else {  newborns = long(randgen->Poisson(step_birthrate * individualHumans.size())); }
            populateNewIndividualsByBirth(newborns);
            break;

            case VitalBirthDependence::DEMOGRAPHIC_DEP_RATE:
            // Birthrate dependent on current census females in correct age range
            if (ind_sampling_type) {  newborns = long(randgen->Poisson(step_birthrate * Possible_Mothers)); }       // KTO !TRACK_ALL?  is this right?
            else {  newborns = long(randgen->Poisson(step_birthrate * Possible_Mothers)); }
            populateNewIndividualsByBirth(newborns);
            break;

            case VitalBirthDependence::INDIVIDUAL_PREGNANCIES:
            case VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE:
            case VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR:
            // Birthrate dependent on current census females in correct age range, who have carried a nine-month pregnancy
            // need to process list of people, count down pregnancy counters, if a birth occurs, call Populate(pointer to mother), then come up with new pregnancies
            for( int iHuman = 0 ; iHuman < individualHumans.size() ; iHuman++ )
            {
                auto individual = individualHumans.at( iHuman );

                if (!individual->IsPossibleMother())
                {
                    continue;
                }

                if (individual->IsPregnant())
                {
                    if (individual->UpdatePregnancy(dt))
                    {
                        populateNewIndividualFromPregnancy(individual);
                    }
                }
                else
                {
                    // If we are using an age-dependent fertility rate, then this needs to be accessed/interpolated based on the current possible-mother's age.
                    if( vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE || 
                        vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR  ) 
                    { 
                        // "FertilityDistribution" is added to map in Node::SetParameters if 'vital_birth_dependence' flag is set to INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE or INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR 
                        float temp_birthrate = 0.0f;
                        if( vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE )
                        {
                            temp_birthrate = GetDemographicsDistribution(NodeDemographicsDistribution::FertilityDistribution)->DrawResultValue(GetUrban(), individual->GetAge());
                        }
                        else    // INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR
                        {
                            temp_birthrate = GetDemographicsDistribution(NodeDemographicsDistribution::FertilityDistribution)->DrawResultValue(individual->GetAge(), float(GetTime().Year()));
                        }

                        LOG_DEBUG_F("%d-year-old possible mother has annual fertility rate = %f\n", int(individual->GetAge()/DAYSPERYEAR), temp_birthrate * DAYSPERYEAR);

                        // In the limit of low birth rate, the probability of becoming pregnant is equivalent to the birth rate.
                        // However, at higher birth rates, some fraction of possible mothers will already be pregnant.  
                        // Roughly speaking, if we want women to give birth every other year, and they gestate for one year,
                        // then the expected time between pregnancy has to be one year, not two.
                        // Hence, the maximum possible birth rate is 1 child per woman per gestation period.
                        if ( temp_birthrate * DAYSPERWEEK * WEEKS_FOR_GESTATION >= 1.0 )
                        {
                            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Check birthrate/vital_birth_dependence mismatch in Node::updateVitalDynamics()" );
                        }
                        else
                        {
                            temp_birthrate /= (1.0F - temp_birthrate * DAYSPERWEEK * WEEKS_FOR_GESTATION);
                        }

                        step_birthrate = temp_birthrate * dt * x_birth;
                    }

                    if (randgen->e() < step_birthrate) 
                    {
                        LOG_DEBUG_F("New pregnancy for %d-year-old\n", int(individual->GetAge()/DAYSPERYEAR));
                        float duration = (DAYSPERWEEK * WEEKS_FOR_GESTATION) - (randgen->e() *dt);
                        individual->InitiatePregnancy( duration );
                    }
                }
            }
            break;
        }
    }

    //------------------------------------------------------------------
    //   Population initialization methods
    //------------------------------------------------------------------

    // This function allows one to scale the initial population by a factor without modifying an input demographics file.
    void Node::PopulateFromDemographics()
    {
        uint32_t InitPop = uint32_t(demographics["NodeAttributes"]["InitialPopulation"].AsUint64());

        // correct initial population if necessary (historical simulation for instance
        if ( GET_CONFIGURABLE(SimulationConfig)->population_scaling )
        {
            InitPop = uint32_t(InitPop * population_scaling_factor);
        }

        populateNewIndividualsFromDemographics(InitPop);
    }

    // This function adds the initial population to the node according to behavior determined by the settings of various flags:
    // (1) ind_sampling_type: TRACK_ALL, FIXED_SAMPLING, ADAPTED_SAMPLING_BY_POPULATION_SIZE, ADAPTED_SAMPLING_BY_AGE_GROUP, ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE
    // (2) demographics_initial, demographics_gender
    // (3) enable_age_initialization, age_initialization_distribution_type
    // (4) vital_birth_dependence: INDIVIDUAL_PREGNANCIES, INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE must have initial pregnancies initialized
    void Node::populateNewIndividualsFromDemographics(int count_new_individuals)
    {
        int32_t num_adults = 0 ;
        int32_t num_children = 0 ;

        // TODO: throw exception on disallowed combinations of parameters (i.e. adaptive sampling without initial demographics)?

        // Set default values for configureAndAddIndividual arguments, sampling rate, etc.
        double temp_age           = 0;
        double initial_prevalence = 0;
        double female_ratio       = 0.5;
        const double default_age  = 20 * DAYSPERYEAR; // age to use by default if age_initialization_distribution_type config parameter is off.

        float temp_sampling_rate  = (ind_sampling_type == IndSamplingType::TRACK_ALL) ? 1.0F : float(Ind_Sample_Rate); // default sampling rate from config if we're doing sampling at all.  modified in case of adapted sampling.

        // Cache pointers to the initial age distribution with the node, so it doesn't have to be created for each individual.
        // After the demographic initialization is complete, it can be removed from the map and deleted
        if( params()->age_initialization_distribution_type == DistributionType::DISTRIBUTION_COMPLEX )
        {
            if( !demographics.Contains( "IndividualAttributes" ) || !demographics["IndividualAttributes"].Contains( "AgeDistribution" ) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Age_Initialization_Distribution_Type", "DISTRIBUTION_COMPLEX", "['IndividualAttributes']['AgeDistribution']", "<not found>" );
            }
            LOG_DEBUG( "Parsing IndividualAttributes->AgeDistribution tag in node demographics file.\n" );
            demographic_distributions[NodeDemographicsDistribution::AgeDistribution] = NodeDemographicsDistribution::CreateDistribution(demographics["IndividualAttributes"]["AgeDistribution"]);
        }

        LOG_DEBUG( "Parsing IndividualAttributes->PrevalenceDistributionFlag tag in node demographics file.\n" );
        auto prevalence_distribution_type = DistributionFunction::Enum(demographics["IndividualAttributes"]["PrevalenceDistributionFlag"].AsInt());
        LOG_DEBUG( "Parsing IndividualAttributes->PrevalenceDistribution1 tag in node demographics file.\n" );
        float prevdist1 = float(demographics["IndividualAttributes"]["PrevalenceDistribution1"].AsDouble());
        LOG_DEBUG( "Parsing IndividualAttributes->PrevalenceDistribution2 tag in node demographics file.\n" );
        float prevdist2 = float(demographics["IndividualAttributes"]["PrevalenceDistribution2"].AsDouble());
        initial_prevalence = Probability::getInstance()->fromDistribution(prevalence_distribution_type, prevdist1, prevdist2, 0);

        // Male/Female ratio implemented as a Gaussian at 50 +/- 1%.
        // TODO: it would be useful to add the possibility to read this from demographics (e.g. as part of enable_age_initialization_distribution by age + gender)
        if (demographics_gender) { female_ratio = randgen->eGauss() * 0.01 + 0.5; }

        // Modify sampling rate in case of adapted sampling by population size
        if ( ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_POPULATION_SIZE ||
             ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE )
        {
            if (count_new_individuals > max_sampling_cell_pop)
            {
                temp_sampling_rate *= max_sampling_cell_pop / count_new_individuals;
            }
        }

        // Keep track of the adapted sampling rate in case it will be further modified on an age-dependent basis for *each* individual in the loop below
        float temp_node_sampling_rate = temp_sampling_rate;

        ExtractDataFromDemographics();

        // Loop over 'count_new_individuals' initial statistical population
        for (int i = 1; i <= count_new_individuals; ++i)
        {
            // For age-dependent adaptive sampling, we need to draw an individual age before adjusting the sampling rate
            if ( ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP || ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE )
            {
                temp_age = calculateInitialAge(default_age);
                temp_sampling_rate = adjustSamplingRateByAge(temp_node_sampling_rate, temp_age);
            }

            // Condition for rejecting potential individuals based on sampling rate in case we're using sampling
            if ( ind_sampling_type != IndSamplingType::TRACK_ALL && randgen->e() > temp_sampling_rate )
            {
                LOG_VALID( "Not creating individual\n" );
                continue;
            }

            // Draw individual's age if we haven't already done it to determine adaptive sampling rate
            if ( (ind_sampling_type != IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP             ) &&
                 (ind_sampling_type != IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE) )
            {
                temp_age = calculateInitialAge(default_age);
                if( demographics["IndividualAttributes"].Contains( "PercentageChildren" ) )
                {
                    float required_percentage_of_children = demographics["IndividualAttributes"]["PercentageChildren"].AsDouble();

                    if( (required_percentage_of_children > 0.0f)  && IndividualHumanConfig::IsAdultAge( 0.0 ) )
                    {
                        throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                            "Minimum_Adult_Age_Years", 0,
                            "<demographics>::Defaults/Node.IndividualAttributes.PercentageChildren", required_percentage_of_children,
                            "You can't have any children if everyone is an adult (Minimum_Adult_Age_Years = 0 implies every one is an adult)." );
                    }

                    float required_percentage_of_adults = 1.0  -required_percentage_of_children ;
                    float percent_children = (float)num_children / (float)count_new_individuals ;
                    float percent_adults   = (float)num_adults   / (float)count_new_individuals ;
                    float age_years = temp_age / DAYSPERYEAR ;

                    // if a child and already have enough children, recalculate age until we get an adult
                    while( !IndividualHumanConfig::IsAdultAge( age_years ) && (percent_children >= required_percentage_of_children) )
                    {
                        temp_age = calculateInitialAge(default_age);
                        age_years = temp_age / DAYSPERYEAR ;
                    }

                    // if an adult and already have enough adults, recalculate age until we get a child
                    while( IndividualHumanConfig::IsAdultAge( age_years ) && (percent_adults >= required_percentage_of_adults) )
                    {
                        temp_age = calculateInitialAge(default_age);
                        age_years = temp_age / DAYSPERYEAR ;
                    }

                    if( IndividualHumanConfig::IsAdultAge( age_years ) )
                    {
                        num_adults++ ;
                    }
                    else
                    {
                        num_children++ ;
                    }
                }
            }

            IIndividualHuman* tempind = configureAndAddNewIndividual(1.0F / temp_sampling_rate, float(temp_age), float(initial_prevalence), float(female_ratio));
            
            if(tempind->GetAge() == 0)
            {
                tempind->setupMaternalAntibodies(nullptr, this);
            }

            // For now, do it unconditionally and see if it can catch all the memory failures cases with minimum cost
            MemoryGauge::CheckMemoryFailure();

            // Every 1000 individuals, do a StatusReport...
            if( individualHumans.size() % 1000 == 0 )
            {
                EnvPtr->getStatusReporter()->ReportInitializationProgress( individualHumans.size(), count_new_individuals );
            }
        }

        // Infection rate needs to be updated for the first timestep
        updateInfectivity(0.0f); 

        // Don't need this distribution after demographic initialization is completed
        // (If we ever want to use it in the future, e.g. in relation to Outbreak ImportCases, we can remove the following.  Clean-up would then be done only in the destructor.)
        if( params()->age_initialization_distribution_type == DistributionType::DISTRIBUTION_COMPLEX )
        {
            EraseAndDeleteDemographicsDistribution(NodeDemographicsDistribution::AgeDistribution);
        }
    }

    void Node::ExtractDataFromDemographics()
    {
        // ----------------------------------------------------------------------------
        // --- The following values are used for each individual for a given node.
        // --- These are extracted here and then used later when creating individuals.
        // ----------------------------------------------------------------------------
        immunity_dist_type = DistributionFunction::NOT_INITIALIZED ;
        immunity_dist1 = 0.0 ;
        immunity_dist2 = 0.0 ;
        risk_dist_type = DistributionFunction::NOT_INITIALIZED ;
        risk_dist1 = 0.0 ;
        risk_dist2 = 0.0 ;

        if (demographics_other)
        {
            // set heterogeneous immunity
            LOG_DEBUG( "Parsing ImmunityDistribution\n" );
            immunity_dist_type = DistributionFunction::Enum(demographics["IndividualAttributes"]["ImmunityDistributionFlag"].AsInt());
            immunity_dist1     =                      float(demographics["IndividualAttributes"]["ImmunityDistribution1"   ].AsDouble());
            immunity_dist2     =                      float(demographics["IndividualAttributes"]["ImmunityDistribution2"   ].AsDouble());

            // set heterogeneous risk
            LOG_DEBUG( "Parsing RiskDistribution\n" );
            risk_dist_type = DistributionFunction::Enum(demographics["IndividualAttributes"]["RiskDistributionFlag"].AsInt());
            risk_dist1     =                      float(demographics["IndividualAttributes"]["RiskDistribution1"   ].AsDouble());
            risk_dist2     =                      float(demographics["IndividualAttributes"]["RiskDistribution2"   ].AsDouble());
        }
    }

    // This function adds newborns to the node according to behavior determined by the settings of various flags:
    // (1) ind_sampling_type: TRACK_ALL, FIXED_SAMPLING, ADAPTED_SAMPLING_BY_POPULATION_SIZE, ADAPTED_SAMPLING_BY_AGE_GROUP, ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE
    // (2) demographics_birth
    // (3) maternal_transmission
    // (4) vital_birth_dependence: FIXED_BIRTH_RATE, POPULATION_DEP_RATE, DEMOGRAPHIC_DEP_RATE. (INDIVIDUAL_PREGNANCIES handled in PopulateNewIndividualFromPregnancy)
    void Node::populateNewIndividualsByBirth(int count_new_individuals)
    {
        // Set default values for configureAndAddIndividual arguments, sampling rate, etc.
        double temp_prevalence    = 0;
        int    temp_infections    = 0;
        double female_ratio       = 0.5;   // TODO: it would be useful to add the possibility to read this from demographics (e.g. in India where there is a significant gender imbalance at birth)

        float  temp_sampling_rate = (ind_sampling_type == IndSamplingType::TRACK_ALL) ? 1.0F : float(Ind_Sample_Rate); // default sampling rate from config if we're doing sampling at all.  modified in case of adapted sampling.

        // Determine the prevalence from which maternal transmission events will be calculated, depending on birth model
        if (maternal_transmission) 
        {
            switch (vital_birth_dependence) 
            {
            case VitalBirthDependence::FIXED_BIRTH_RATE:
            case VitalBirthDependence::POPULATION_DEP_RATE:
                temp_prevalence = (statPop > 0) ? float(Infected) / statPop : 0; break;
            case VitalBirthDependence::DEMOGRAPHIC_DEP_RATE:
                temp_prevalence = getPrevalenceInPossibleMothers(); break;

            case VitalBirthDependence::INDIVIDUAL_PREGNANCIES: break;
            case VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE: break;
            case VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR: break;
            default: break;
            }
        }

        // For births, the adapted sampling by age uses the 'sample_rate_birth' parameter
        if (ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP || ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE)
            temp_sampling_rate *= sample_rate_birth;

        // Modify sampling rate according to population size if so specified
        if (ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_POPULATION_SIZE || ind_sampling_type == IndSamplingType::ADAPTED_SAMPLING_BY_AGE_GROUP_AND_POP_SIZE)
            if (statPop > max_sampling_cell_pop) { temp_sampling_rate *= max_sampling_cell_pop / statPop; }

        if (temp_sampling_rate > 1) 
        { 
            LOG_WARN("temp_sampling_rate was greater than 1?!  Are you sure sample_rate_birth, ind_sampling_rate, etc. were specified correctly?\n");
            temp_sampling_rate = 1; 
        }

        // Loop through potential new births 
        for (int i = 1; i <= count_new_individuals; i++)
        {
            // Condition for rejecting potential individuals based on sampling rate in case we're using sampling
            if ( ind_sampling_type != IndSamplingType::TRACK_ALL && randgen->e() >= temp_sampling_rate ) continue;

            // Configure and/or add new individual (EAW: there doesn't appear to be any significant difference between the two cases below)
            auto prob_maternal_transmission = GET_CONFIGURABLE(SimulationConfig)->prob_maternal_transmission;
            IIndividualHuman* child = nullptr;
            if (demographics_birth)
            {
                child = configureAndAddNewIndividual(1.0F / temp_sampling_rate, 0, float(temp_prevalence) * prob_maternal_transmission, float(female_ratio)); // N.B. temp_prevalence=0 without maternal_transmission flag
            }
            else
            {
                if (maternal_transmission && randgen->e() < temp_prevalence * prob_maternal_transmission)
                { 
                    temp_infections = 1;
                }

                child = addNewIndividual(1.0F / temp_sampling_rate, 0.0F, randgen->i(Gender::COUNT),
                    temp_infections, 1.0F, 1.0F, 1.0F, 1.0F * (randgen->e() < Above_Poverty));
            }

            child->setupMaternalAntibodies(nullptr, this);
            Births += 1.0f / temp_sampling_rate;
        }
    }

    // Populate with an Individual as an argument
    // This individual is the mother, and will give birth to a child under vital_birth_dependence==INDIVIDUAL_PREGNANCIES
    void Node::populateNewIndividualFromPregnancy(IIndividualHuman* mother)
    {
        //  holds parameters for new birth
        int child_infections = 0;
        float child_age      = 0;
        int child_gender     = Gender::MALE;
        float mc_weight      = float(mother->GetMonteCarloWeight()); // same sampling weight as mother
        float child_poverty  = float(mother->GetAbovePoverty()); //same poverty as mother //note, child's financial level independent of mother, may be important for diseases with a social network structure
        float female_ratio   = 0.5;



        if (randgen->e() < female_ratio) { child_gender = Gender::FEMALE; }

        if (maternal_transmission)
        {
            if (mother->IsInfected())
            {
                auto actual_prob_maternal_transmission = mother->getProbMaternalTransmission();
                if (randgen->e() < actual_prob_maternal_transmission)
                {
                    LOG_DEBUG_F( "Mother transmitting infection to newborn.\n" );
                    child_infections = 1;
                }
            }
        }

        IIndividualHuman *child = addNewIndividual(mc_weight, child_age, child_gender, child_infections, 1.0, 1.0, 1.0, child_poverty);
        auto context = dynamic_cast<IIndividualHumanContext*>(mother);
        child->setupMaternalAntibodies(context, this);
        Births += mc_weight;//  Born with age=0 and no infections and added to sim with same sampling weight as mother
    }

    float Node::getPrevalenceInPossibleMothers()
    {
        float prevalence = 0;
        if (Possible_Mothers == 0) return prevalence; // prevent divide by zero up here

        for (auto individual : individualHumans)
        {
            if (individual->IsPossibleMother() && individual->IsInfected())
            {
                prevalence += float(individual->GetMonteCarloWeight());
            }
        }

        // Normalize to get prevalence in possible mothers
        prevalence /= Possible_Mothers;

        return prevalence;
    }

    void Node::conditionallyInitializePregnancy(IIndividualHuman* individual)
    {
        float duration;
        float temp_birthrate = birthrate;

        if (individual->IsPossibleMother()) // woman of child-bearing age?
        {
            if(vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE) 
            { 
                // "FertilityDistribution" is added to map in Node::SetParameters if 'vital_birth_dependence' flag is set to INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE

                temp_birthrate = GetDemographicsDistribution(NodeDemographicsDistribution::FertilityDistribution)->DrawResultValue((GetUrban()?1.0:0.0), individual->GetAge());
                LOG_DEBUG_F("%d-year-old possible mother has annual fertility rate = %f\n", int(individual->GetAge()/DAYSPERYEAR), temp_birthrate * DAYSPERYEAR);
            }
            else if(vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR) 
            { 
                // "FertilityDistribution" is added to map in Node::SetParameters if 'vital_birth_dependence' flag is set to INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR

                temp_birthrate = GetDemographicsDistribution(NodeDemographicsDistribution::FertilityDistribution)->DrawResultValue(individual->GetAge(), float(GetTime().Year()));
                LOG_DEBUG_F("%d-year-old possible mother has annual fertility rate = %f\n", int(individual->GetAge()/DAYSPERYEAR), temp_birthrate * DAYSPERYEAR);
            }

            if (randgen->e() < x_birth * temp_birthrate * (DAYSPERWEEK * WEEKS_FOR_GESTATION)) // is the woman within any of the 40 weeks of pregnancy?
            {
                duration = float(randgen->e()) * (DAYSPERWEEK * WEEKS_FOR_GESTATION); // uniform distribution over 40 weeks
                LOG_DEBUG_F("Initial pregnancy of %f remaining days for %d-year-old\n", duration, (int)(individual->GetAge()/DAYSPERYEAR));
                individual->InitiatePregnancy(duration);// initialize the pregnancy
            }
        }
    }

    IIndividualHuman* Node::createHuman(suids::suid id, float monte_carlo_weight, float initial_age, int gender, float isabovepoverty)
    {
        return IndividualHuman::CreateHuman(this, id, monte_carlo_weight, initial_age, gender, isabovepoverty);
    }

    float Node::drawInitialImmunity(float ind_init_age)
    {
        float temp_immunity = 1.0;

        switch( params()->immunity_initialization_distribution_type ) 
        {
        case DistributionType::DISTRIBUTION_COMPLEX:
            {
            // Read from "ImmunityDistribution" that would have been added to map in Node::LoadImmunityDemographicsDistribution
            float randdraw = randgen->e();
            temp_immunity = GetDemographicsDistribution(NodeDemographicsDistribution::ImmunityDistribution)->DrawFromDistribution(randdraw, ind_init_age);
            LOG_DEBUG_F( "creating individual with age = %f and immunity = %f, with randdraw = %f\n",  ind_init_age, temp_immunity, randdraw);
            }
            break;

        case DistributionType::DISTRIBUTION_SIMPLE:
            temp_immunity = float(Probability::getInstance()->fromDistribution(immunity_dist_type, immunity_dist1, immunity_dist2, 1.0));
            break;

        case DistributionType::DISTRIBUTION_OFF:
            // For backward compatibility and avoiding surprises if Enable_Demographics_Other=1 but ImmunityDistribution1=0 (ERAD-2747)
            temp_immunity = 1.0;
            break;

        default:
            if( !JsonConfigurable::_dryrun )
            {
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "immunity_initialization_distribution_type", params()->immunity_initialization_distribution_type, DistributionType::pairs::lookup_key( params()->immunity_initialization_distribution_type ) );
            }
        }

        return temp_immunity;
    }

    IIndividualHuman* Node::configureAndAddNewIndividual(float ind_MCweight, float ind_init_age, float comm_init_prev, float comm_female_ratio)
    {
        //  Holds draws from demographic distribution
        int   temp_infs      = 0;
        int   temp_gender    = Gender::MALE;
        float temp_poverty   = 0;
        float temp_immunity  = 1.0;
        float temp_risk      = 1.0;
        float temp_migration = 1.0;

        // gender distribution exists regardless of gender demographics, but is ignored unless vital_birth_dependence is 2 or 3
        if (randgen->e() < comm_female_ratio)
        { 
            temp_gender = Gender::FEMALE;
        }

        // if individual *could* be infected, do a random draw to determine his/her initial infected state
        if (comm_init_prev > 0 && randgen->e() < comm_init_prev)
        { 
            temp_infs = 1;
        }

        if (demographics_other)
        {
            // set initial immunity (or heterogeneous innate immunity in derived malaria code)
            temp_immunity = drawInitialImmunity(ind_init_age);

            // set heterogeneous risk
            temp_risk = float(Probability::getInstance()->fromDistribution(risk_dist_type, risk_dist1, risk_dist2, 1.0));
        }

        if( (migration_info != nullptr) && migration_info->IsHeterogeneityEnabled() )
        {
            // This is not done during initialization but other times when the individual is created.           
            temp_migration = float(Probability::getInstance()->fromDistribution(migration_dist_type, migration_dist1, migration_dist2, 1.0));
        }

        // Finally, add the individual
        if (randgen->e() < Above_Poverty)
        { 
            temp_poverty = 1.0; 
        }

        IIndividualHuman* tempind = addNewIndividual(ind_MCweight, ind_init_age, temp_gender, temp_infs, temp_immunity, temp_risk, temp_migration, temp_poverty);

        // Now if tracking individual pregnancies, need to see if this new Individual is pregnant to begin the simulation
        if ( vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES ||
             vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE ||
             vital_birth_dependence == VitalBirthDependence::INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR )
        { 
            conditionallyInitializePregnancy(tempind);
        }

        return tempind;
    }

    IIndividualHuman* Node::addNewIndividual(float mc_weight, float initial_age, 
        int gender, int initial_infections, float immunity_parameter, float risk_parameter, 
        float migration_heterogeneity, float poverty_parameter)
    {

        // new creation process
        IIndividualHuman* new_individual = createHuman(parent->GetNextIndividualHumanSuid(), mc_weight, initial_age, gender, poverty_parameter); // initial_infections isn't needed here if SetInitialInfections function is used

        // EAW: is there a reason that the contents of the two functions below aren't absorbed into CreateHuman?  this whole process seems very convoluted.
        new_individual->SetParameters(1.0, immunity_parameter, risk_parameter, migration_heterogeneity);// default values being used except for total number of communities
        new_individual->SetInitialInfections(initial_infections);
        new_individual->UpdateGroupMembership();
        new_individual->UpdateGroupPopulation(1.0f);

        individualHumans.push_back(new_individual);
        home_individual_ids.insert( std::make_pair( new_individual->GetSuid().data, new_individual->GetSuid() ) );

        event_context_host->TriggerNodeEventObservers( new_individual->GetEventContext(), IndividualEventTriggerType::Births ); // EAW: this is not just births!!  this will also trigger on e.g. AddImportCases

        if( new_individual->GetParent() == nullptr )
        {
            LOG_INFO( "In addNewIndividual, indiv had no context, setting (migration hack path)\n" );
            new_individual->SetContextTo( this );
        }
        LOG_DEBUG_F("Added individual %d to node %d.\n", new_individual->GetSuid().data, GetSuid().data );

        new_individual->InitializeHuman();

        return new_individual;
    }

    IIndividualHuman* Node::addNewIndividualFromSerialization()
    {
        LOG_DEBUG_F( "1. %s\n", __FUNCTION__ );
        IIndividualHuman* new_individual = createHuman( suids::nil_suid(), 0, 0, 0, 0);
        new_individual->SetParameters(1.0, 0, 0, 0);// default values being used except for total number of communities

#if 0
        new_individual->SetInitialInfections(0);

        // Set up transmission groups
        if (params()->heterogeneous_intranode_transmission_enabled) 
        {
            new_individual->UpdateGroupMembership();
        }
#endif
        //individualHumans.push_back(new_individual);
#if 0
        event_context_host->TriggerIndividualEventObservers( new_individual->GetEventContext(), IndividualEventTriggerType::Births ); // EAW: this is not just births!!  this will also trigger on e.g. AddImportCases

        if( new_individual->GetParent() == nullptr )
        {
            LOG_INFO( "In addNewIndividual, indiv had no context, setting (migration hack path)\n" );
            new_individual->SetContextTo( this );
        }
#endif
        //processImmigratingIndividual( new_individual );
        LOG_DEBUG_F( "addNewIndividualFromSerialization,individualHumans size: %d, ih context=%p\n",individualHumans.size(), new_individual->GetParent() );

        return new_individual;
    }


    double Node::calculateInitialAge(double age)
    {
        // Set age from distribution, or if no proper distribution set, make all initial individuals 20 years old (7300 days)

        if(params()->age_initialization_distribution_type == DistributionType::DISTRIBUTION_COMPLEX)
        {
            // "AgeDistribution" is added to map in Node::SetParameters if 'enable_age_initialization_distribution' flag is set
            age = GetDemographicsDistribution(NodeDemographicsDistribution::AgeDistribution)->DrawFromDistribution(randgen->e());
        }
        else if (params()->age_initialization_distribution_type == DistributionType::DISTRIBUTION_SIMPLE)
        {
            if( !demographics.Contains( "IndividualAttributes" ) ||
                !demographics["IndividualAttributes"].Contains( "AgeDistributionFlag" ) ||
                !demographics["IndividualAttributes"].Contains( "AgeDistribution1" ) ||
                !demographics["IndividualAttributes"].Contains( "AgeDistribution2" ) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Age_Initialization_Distribution_Type", "DISTRIBUTION_SIMPLE", "['IndividualAttributes']['AgeDistributionFlag' or 'AgeDistribution1' or 'AgeDistribution2']", "<not found>" );
            }
            LOG_DEBUG( "Parsing IndividualAttributes->AgeDistributionFlag tag in node demographics file.\n" );
            auto age_distribution_type = DistributionFunction::Enum(demographics["IndividualAttributes"]["AgeDistributionFlag"].AsInt());
            LOG_DEBUG( "Parsing IndividualAttributes->AgeDistribution1 tag in node demographics file.\n" );
            float agedist1 = float(demographics["IndividualAttributes"]["AgeDistribution1"].AsDouble()); 
            LOG_DEBUG( "Parsing IndividualAttributes->AgeDistribution2 tag in node demographics file.\n" );
            float agedist2 = float(demographics["IndividualAttributes"]["AgeDistribution2"].AsDouble());

            age = Probability::getInstance()->fromDistribution(age_distribution_type, agedist1, agedist2, (20 * DAYSPERYEAR));
        }

        return age;
    }

    Fraction
    Node::adjustSamplingRateByAge(
        Fraction sampling_rate,
        double age
    )
    const
    {
        if (age < (18 * IDEALDAYSPERMONTH)) { sampling_rate *= sample_rate_0_18mo; }
        else if (age <  (5 * DAYSPERYEAR))  { sampling_rate *= sample_rate_18mo_4yr; }
        else if (age < (10 * DAYSPERYEAR))  { sampling_rate *= sample_rate_5_9; }
        else if (age < (15 * DAYSPERYEAR))  { sampling_rate *= sample_rate_10_14; }
        else if (age < (20 * DAYSPERYEAR))  { sampling_rate *= sample_rate_15_19; }
        else { sampling_rate *= sample_rate_20_plus; }

        // Now correct sampling rate, in case it is over 100 percent
        if (sampling_rate > 1) 
        { 
            LOG_WARN("Sampling_rate was greater than 1?!  Are you sure Ind_Sample_Rate and age-dependent rates were specified correctly?\n");
            sampling_rate = 1; 
        }

        return sampling_rate;
    }
    
    Fraction
    Node::adjustSamplingRateByImmuneState(
        Fraction sampling_rate,
        bool is_immune
    )
    const
    {
        Fraction tmp = sampling_rate;
        if (is_immune)
        {
            sampling_rate *= sample_rate_immune;
        }
        //else { sampling_rate *= 1; }

        // Now correct sampling rate, in case it is over 100 percent
        if (sampling_rate > 1) 
        { 
            LOG_WARN("Sampling_rate was greater than 1?!  Are you sure Ind_Sample_Rate and age-dependent rates were specified correctly?\n");
            sampling_rate = 1; 
        }

        LOG_DEBUG_F( "%s: sampling_rate in = %f, is_immune = %d, sampling_route out = %f\n", __FUNCTION__, (float) tmp, is_immune, (float) sampling_rate );
        return sampling_rate;
    }
    
    void Node::EraseAndDeleteDemographicsDistribution(std::string key)
    {
        NodeDemographicsDistribution* ndd;
        std::map<std::string, NodeDemographicsDistribution*>::iterator it = demographic_distributions.find(key);
        if( it != demographic_distributions.end() )
        {
            ndd = it->second;
            demographic_distributions.erase(it);
            delete ndd;
        }
        else
        {
            std::ostringstream errMsg;
            errMsg << "EraseAndDeleteDemographicsDistribution() cannot find key '" << key << "' in demographic_distributions map" << std::endl;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
        }
    }

    //------------------------------------------------------------------
    //   Individual migration methods
    //------------------------------------------------------------------

    void Node::processEmigratingIndividual(IIndividualHuman* individual)
    {
        release_assert( individual );

        // hack for now: legacy components of departure code
        resolveEmigration(individual);

        individual->SetContextTo(nullptr);
        postIndividualMigration(individual);
    }

    void Node::postIndividualMigration(IIndividualHuman* individual)
    {
        parent->PostMigratingIndividualHuman(individual);
    }

    void Node::resolveEmigration(IIndividualHuman* individual)
    {
        LOG_DEBUG_F( "individual %lu is leaving node %lu and going to %lu\n", individual->GetSuid().data, GetSuid().data,
            individual->GetMigrationDestination().data );

        event_context_host->TriggerNodeEventObservers( individual->GetEventContext(), IndividualEventTriggerType::Emigrating );

        // Do emigration logic here
        // Handle departure-linked interventions for individual
        if (params()->interventions ) // && departure_linked_dist)
        {
            event_context_host->ProcessDepartingIndividual(individual);
        }
    }

    IIndividualHuman* Node::processImmigratingIndividual(IIndividualHuman* movedind)
    {
        if( movedind->IsDead() )
        {
            // -------------------------------------------------------------
            // --- We want individuals to officially die in their home node
            // -------------------------------------------------------------
            movedind->SetContextTo(getContextPointer());
            release_assert( movedind->AtHome() );

            home_individual_ids.erase( movedind->GetSuid().data );
        }
        else
        {
            individualHumans.push_back(movedind);
            movedind->SetContextTo(getContextPointer());

            // check for arrival-linked interventions BEFORE!!!! setting the next migration
            if (params()->interventions )
            {
                event_context_host->ProcessArrivingIndividual(movedind);
            }
            event_context_host->TriggerNodeEventObservers( movedind->GetEventContext(), IndividualEventTriggerType::Immigrating );

            movedind->UpdateGroupMembership();
            movedind->UpdateGroupPopulation(1.0f);
        }
        return movedind;
    }

    bool Node::IsEveryoneHome() const
    {
        if( individualHumans.size() < home_individual_ids.size() )
        {
            // someone is missing
            return false ;
        }
        // there could be more people in the node than call it home

        int num_people_found = 0 ;
        for( auto individual : individualHumans )
        {
            if( home_individual_ids.count( individual->GetSuid().data ) > 0 )
            {
                num_people_found++ ;
                if( num_people_found == home_individual_ids.size() )
                {
                    return true ;
                }
            }
        }
        return false ;
    }

    //------------------------------------------------------------------
    //   Reporting methods
    //------------------------------------------------------------------

    void Node::resetNodeStateCounters()
    {
        Possible_Mothers       = 0;
        statPop                = 0;  // Population for statistical purposes
        Infected               = 0;
        mInfectivity           = 0;
        new_infections         = 0;
        new_reportedinfections = 0;
        newInfectedPeopleAgeProduct = 0;
    }

    void Node::updateNodeStateCounters(IIndividualHuman* ih)
    {
        switch(ih->GetNewInfectionState()) 
        { 
        case NewInfectionState::NewlyDetected: 
            reportDetectedInfection(ih);
            break; 

        case NewInfectionState::NewAndDetected: 
            reportDetectedInfection(ih);
            reportNewInfection(ih);
            break; 

        case NewInfectionState::NewInfection: 
            reportNewInfection(ih);
            break;

        case NewInfectionState::Invalid: break;
        case NewInfectionState::NewlyActive: break;
        case NewInfectionState::NewlyInactive: break;
        case NewInfectionState::NewlyCleared: break;
        default: break;
        } 

        ih->ClearNewInfectionState();
    }

    void Node::reportNewInfection(IIndividualHuman *ih)
    {
        float monte_carlo_weight = float(ih->GetMonteCarloWeight());

        new_infections += monte_carlo_weight; 
        Cumulative_Infections += monte_carlo_weight; 
        event_context_host->TriggerNodeEventObservers( ih->GetEventContext(), IndividualEventTriggerType::NewInfectionEvent );

        newInfectedPeopleAgeProduct += monte_carlo_weight * float(ih->GetAge());
    }

    void Node::reportDetectedInfection(IIndividualHuman *ih)
    {
        float monte_carlo_weight = float(ih->GetMonteCarloWeight());

        new_reportedinfections += monte_carlo_weight; 
        Cumulative_Reported_Infections += monte_carlo_weight; 
    }

    void Node::finalizeNodeStateCounters(void)
    {
        infected_people_prior.push_back( float(new_infections) );
        if( infected_people_prior.size() > infection_averaging_window )
        {
            infected_people_prior.pop_front();
        }
        if( newInfectedPeopleAgeProduct < 0 )
        {
            throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "newInfectedPeopleAgeProduct", newInfectedPeopleAgeProduct, 0 );
        }

        infected_age_people_prior.push_back( float(newInfectedPeopleAgeProduct) );
        if( infected_age_people_prior.size() > infection_averaging_window )
        {
            infected_age_people_prior.pop_front();
        }

        double numerator = std::accumulate(infected_age_people_prior.begin(), infected_age_people_prior.end(), 0.0);
        if( numerator < 0.0 )
        {
            throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "numerator", numerator, 0 );
        }

        float denominator = std::accumulate( infected_people_prior.begin(), infected_people_prior.end(), 0 );
        if( denominator && numerator )
        {
            mean_age_infection = numerator/( denominator * DAYSPERYEAR);
            LOG_DEBUG_F( "mean_age_infection = %f/%f*365.\n", numerator, denominator );
        }
        else
        {
            mean_age_infection = 0; // necessary? KM comment - yes, if numerator = 0 then normal calc is OK; if denom is 0 (say, in an eradication context), then above calc will throw Inf/NaN/exception, depending on divide-by-zero handling.
        }



    }

    //------------------------------------------------------------------
    //   Campaign event related
    //------------------------------------------------------------------

    void Node::AddEventsFromOtherNodes( const std::vector<std::string>& rEventNameList )
    {
        events_from_other_nodes.clear();
        for( auto event_name : rEventNameList )
        {
            events_from_other_nodes.push_back( event_name );
        }
    }

    // Determines if Node is in a defined lat-long polygon
    // checks for line crossings when extending a ray from the Node's location to increasing longitude
    // odd crosses included, even crossed excluded

    bool Node::IsInPolygon(float* vertex_coords, int numcoords)
    {
        bool inside = false;

        if (numcoords < 6) { return 0; }//no inclusion in a line or point
        if (numcoords % 2 != 0)
        {
            std::stringstream s ;
            s << "Error parsing polygon inclusion: numcords(=" << numcoords << ") is not even." << std::endl ;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
        }

        float lat = GetLatitudeDegrees() ;
        float lon = GetLongitudeDegrees();
        for (int i = 0; i < numcoords / 2 - 1; i++) // increase to the second to last coordinate pair
        {
            // first check if latitude is between the vertex latitude coordinates
            if ((lat < vertex_coords[2 * i + 1]) != (lat < vertex_coords[2 * i + 3])) // also prevents divide by zero
            {
                if (lon < (vertex_coords[2 * i + 2] - vertex_coords[2 * i]) * (lat - vertex_coords[2 * i + 1]) / (vertex_coords[2 * i + 3] - vertex_coords[2 * i + 1]) + vertex_coords[2 * i])
                {
                    inside = !inside;   // crossing!
                }
            }
        }

        return inside;
    }

    // TODO: Cache results so we don't recalculate all the time. (that's why it's non-const)
    bool Node::IsInPolygon( const json::Array &poly )
    {
        bool inside = false;

        // Assume polygon was validated prior to us by NodeSet class for having at least 3 vertices.
        Array::const_iterator itPoly( poly.Begin()), itPolyEnd( poly.End() );
        QuickInterpreter poly_interp(poly);
        int i = 0;
        float lat = GetLatitudeDegrees() ;
        float lon = GetLongitudeDegrees();
        for (; itPoly != itPolyEnd; ++itPoly) // increase to the second to last coordinate pair
        {
            // first check if latitude is between the vertex latitude coordinates
            //if ((ndc->latitude < vertex_coords[2 * i + 1]) != (ndc->latitude < vertex_coords[2 * i + 3])) // also prevents divide by zero
            if (lat < float(poly_interp[i][1].As<Number>()) != (lat < float(poly_interp[i+1][1].As<Number>()))) // also prevents divide by zero
            {
                //if (ndc->longitude < (vertex_coords[2 * i + 2] - vertex_coords[2 * i]) * (ndc->latitude - vertex_coords[2 * i + 1]) / (vertex_coords[2 * i + 3] - vertex_coords[2 * i + 1]) + vertex_coords[2 * i])
                // for clarity
                float curLong  = float(poly_interp[i  ][0].As<Number>());
                float curLat   = float(poly_interp[i  ][1].As<Number>());
                float nextLong = float(poly_interp[i+1][0].As<Number>());
                float nextLat  = float(poly_interp[i+1][1].As<Number>());
                if (lon < (nextLong - curLong) * (lat - curLat) / (nextLat - curLat) + curLong)
                {
                    inside = !inside;   // crossing!
                }
            }
            i++;
        }
        return inside;
    }

    bool Node::IsInExternalIdSet( const tNodeIdList &nodelist )
    {
        if( std::find( nodelist.begin(), nodelist.end(), externalId ) == nodelist.end() )
        {
            return false;
        }

        return true;
    }

    void Node::RemoveHuman( int index )
    {
        LOG_DEBUG_F( "Purging individual %d from node.\n", individualHumans[ index ]->GetSuid().data );
        individualHumans[ index ] = individualHumans.back();
        individualHumans.pop_back();
    }

    //------------------------------------------------------------------
    //   Assorted getters and setters
    //------------------------------------------------------------------

    const NodeDemographicsDistribution* Node::GetDemographicsDistribution(std::string key) const
    {
        std::map<std::string, NodeDemographicsDistribution*>::const_iterator it = demographic_distributions.find(key);

        if( it == demographic_distributions.end() )
        {
            std::ostringstream errMsg;
            errMsg << "GetDemographicsDistribution() cannot find key '" << key << "' in demographic_distributions map" << std::endl;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
        }
        else
        {
            return it->second; 
        }
    }

    std::vector<bool> Node::GetMigrationTypeEnabledFromDemographics() const
    {
        std::vector<bool> demog_enabled ;

        demog_enabled.push_back( true ) ; // local
        demog_enabled.push_back( demographics["NodeAttributes"]["Airport"].AsUint64() != 0 );
        demog_enabled.push_back( demographics["NodeAttributes"]["Region" ].AsUint64() != 0 );
        demog_enabled.push_back( demographics["NodeAttributes"]["Seaport"].AsUint64() != 0 );
        demog_enabled.push_back( true ) ; // family

        return demog_enabled ;
    }


    ::RANDOMBASE* Node::GetRng()
    {
        release_assert(parent);
        if( parent == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "parent", "ISimulationContext*" );
        }
        return parent->GetRng();
    }

    void Node::propagateContextToDependents()
    {
        INodeContext *context = getContextPointer();

        for (auto individual : individualHumans)
        {
            individual->SetContextTo(context);
        }
#ifndef DISABLE_CLIMATE
        if (localWeather)
        {
            localWeather->SetContextTo(context);
        }
#endif
        if (migration_info)
        {
            migration_info->SetContextTo(context);
        }

        if (event_context_host)
        {
            event_context_host->SetContextTo(context); 
        }
    }

    void Node::SetContextTo(ISimulationContext* context)
    {
        parent = context;
        propagateContextToDependents();
        demographics.SetContext( parent->GetDemographicsContext(), (INodeContext*)(this) );
    }

    // INodeContext methods
    ISimulationContext* Node::GetParent() { return parent; }
    suids::suid Node::GetSuid() const { return suid; }
    suids::suid Node::GetNextInfectionSuid() { return parent->GetNextInfectionSuid(); }

    IMigrationInfo* Node::GetMigrationInfo() { return migration_info; }
    const NodeDemographics* Node::GetDemographics() const { return &demographics; }

    // Methods for implementing time dependence in infectivity, birth rate, migration, etc.
    float Node::getSinusoidalCorrection(float sinusoidal_amplitude, float sinusoidal_phase) const
    {
        float correction = 1 + sinusoidal_amplitude * sin(2.0 * PI * (GetTime().time - sinusoidal_phase) / DAYSPERYEAR );
        LOG_DEBUG_F( "%s returning %f\n", __FUNCTION__, correction );
        return correction;
    }

    float Node::getBoxcarCorrection(float boxcar_amplitude, float boxcar_start_time, float boxcar_end_time) const
    {
        float correction = 1;
        //Handle cases in which start_time is earlier than end_time, and when end_time is earlier than start_time (i.e., when the high season extends over adjacent years)
        float modTime = fmod(GetTime().time, DAYSPERYEAR);
        
        if( (boxcar_start_time < boxcar_end_time) && (modTime > boxcar_start_time) && (modTime < boxcar_end_time) )
        {
            correction = 1 + boxcar_amplitude;
        }
        if( (boxcar_end_time < boxcar_start_time) &&  ( (modTime < boxcar_end_time) || (modTime > boxcar_start_time) ) )
        {
            correction = 1 + boxcar_amplitude;
        }
        return correction;
    }

    // Reporting methods
    IdmDateTime
    Node::GetTime()          const { return parent->GetSimulationTime(); }

    float
    Node::GetInfected()      const { return Infected; }

    float
    Node::GetStatPop()       const { return statPop; }

    float
    Node::GetBirths()        const { return Births; }

    float
    Node::GetCampaignCost()  const { return Campaign_Cost; }

    float
    Node::GetInfectivity()   const { return mInfectivity; }

    float
    Node::GetInfectionRate() const { return infectionrate; }

    float
    Node::GetSusceptDynamicScaling() const { return susceptibility_dynamic_scaling; }

    ExternalNodeId_t
    Node::GetExternalID()    const { return externalId; }

    const Climate*
    Node::GetLocalWeather()    const { return localWeather; }

    long int
    Node::GetPossibleMothers() const { return Possible_Mothers ; }

    bool 
    Node::GetUrban()           const { return urban; }

    float
    Node::GetMeanAgeInfection()      const { return mean_age_infection; }

    INodeEventContext* Node::GetEventContext() { return (INodeEventContext*)event_context_host; }

    INodeContext *Node::getContextPointer()    { return this; }

    float Node::GetBasePopulationScaleFactor() const
    {
        return population_scaling_factor;
    }

    const SimulationConfig* Node::params() const
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    const tPropertiesDistrib&
    Node::GetIndividualPropertyDistributions() const
    {
        return distribs;
    }

    bool Node::IsValidTransmissionRoute( string& transmissionRoute )
    {
        std::transform(transmissionRoute.begin(), transmissionRoute.end(), transmissionRoute.begin(), ::tolower);
        bool isValid = ((transmissionRoute == "contact") || (transmissionRoute == "environmental"));
        return isValid;
    }

    void Node::ValidateIntranodeTransmissionConfiguration()
    {
        bool oneOrMoreMatrices = false;

        const NodeDemographics& properties = demographics[ IP_KEY ];
        for (int iProperty = 0; iProperty < properties.size(); iProperty++) {

            const NodeDemographics& property = properties[ iProperty ];
            if (property.Contains(TRANSMISSION_MATRIX_KEY)) {

                const NodeDemographics& transmissionMatrix = property[ TRANSMISSION_MATRIX_KEY ];

                //check route
                if (transmissionMatrix.Contains(ROUTE_KEY)) {
                    string transmissionRoute = transmissionMatrix[ ROUTE_KEY ].AsString();
                    if (!IsValidTransmissionRoute(transmissionRoute))
                    {
                        ostringstream message;
                        message << "HINT Configuration: Unsupported route '" << transmissionMatrix[ ROUTE_KEY ].AsString() << "'." << endl;
                        message << "For generic/TB sims, only \"contact\" route (contagion is reset at each timestep) is supported." << endl;
                        message << "For environmental/polio sims, we support \"contact\" (contagion is reset at each timestep) and \"environmental\" (fraction of contagion carried over to next time step = 1 - Node_Contagion_Decay_Rate." << endl;
                        message << "For malaria sims, transmissionMatrix is not supported (yet)." << endl;
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, message.str().c_str());
                    }
                }
                else {
                    LOG_WARN_F("HINT Configuration: Missing 'Route' for property '%s'. Will use default route 'contact' (contagion is reset at each timestep).\n", property[IP_NAME_KEY].AsString().c_str());
                }

                oneOrMoreMatrices = true;
                int valueCount;

                string propertyName = property[IP_NAME_KEY].AsString();
                if (propertyName == _age_bins_key) {

                    valueCount = distribs[ _age_bins_key ].size();
                    if (valueCount == 0) {
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "HINT Configuration: Expected one or more age bins.");
                    }
                }
                else {

                    const NodeDemographics& propertyValues = property[ IP_VALUES_KEY ];
                    valueCount = propertyValues.size();
                    if (valueCount == 0) {
                        ostringstream message;
                        message << "HINT Configuration: Expected one or more property values for property '" << propertyName << "'.";
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, message.str().c_str());
                    }
                }

                const NodeDemographics& scalingMatrixRows = transmissionMatrix[TRANSMISSION_DATA_KEY];
                if (scalingMatrixRows.size() != valueCount) {
                    ostringstream message;
                    message << "HINT Configuration: Transmission matrix for property '" << propertyName << "' has incorrect number of rows - " << scalingMatrixRows.size() << " (expected " << valueCount << ").";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, message.str().c_str());
                }

                for (int iRow = 0; iRow < valueCount; iRow++) {

                    const NodeDemographics& scalingMatrixRow = scalingMatrixRows[ iRow ];
                    if (scalingMatrixRow.size() != valueCount) {
                        ostringstream message;
                        message << "HINT Configuration: Transmission matrix row " << iRow << " has incorrect number of columns - " << scalingMatrixRow.size() << " (expected " << valueCount << ").";
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, message.str().c_str());
                    }
                }
            }
        }

        if (!oneOrMoreMatrices) {
            LOG_WARN("HINT Configuration: heterogeneous intranode transmission is enabled, but no transmission matrices were found in the demographics file(s).\n");
            // TODO throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "HINT Configuration: No transmission matrices were found in the demographics file(s).");
        }
    }

    REGISTER_SERIALIZABLE(Node);

    void Node::serialize(IArchive& ar, Node* obj)
    {
        Node& node = *obj;
        ar.labelElement("serializationMask") & (uint32_t&)node.serializationMask;

        if ((node.serializationMask & SerializationFlags::Population) != 0) {
            ar.labelElement("individualHumans"   ) & node.individualHumans;
            ar.labelElement("home_individual_ids") & node.home_individual_ids;
        }

        if ((node.serializationMask & SerializationFlags::Parameters) != 0) {
            ar.labelElement("ind_sampling_type") & (uint32_t&)node.ind_sampling_type;
            ar.labelElement("population_density_infectivity_correction") & (uint32_t&)node.population_density_infectivity_correction;

            ar.labelElement("demographics_birth") & node.demographics_birth;
            ar.labelElement("demographics_gender") & node.demographics_gender;
            ar.labelElement("demographics_other") & node.demographics_other;
            ar.labelElement("max_sampling_cell_pop") & node.max_sampling_cell_pop;
            ar.labelElement("sample_rate_birth") & node.sample_rate_birth;
            ar.labelElement("sample_rate_0_18mo") & node.sample_rate_0_18mo;
            ar.labelElement("sample_rate_18mo_4yr") & node.sample_rate_18mo_4yr;
            ar.labelElement("sample_rate_5_9") & node.sample_rate_5_9;
            ar.labelElement("sample_rate_10_14") & node.sample_rate_10_14;
            ar.labelElement("sample_rate_15_19") & node.sample_rate_15_19;
            ar.labelElement("sample_rate_20_plus") & node.sample_rate_20_plus;
            ar.labelElement("sample_rate_immune") & node.sample_rate_immune;
            ar.labelElement("immune_threshold_for_downsampling") & node.immune_threshold_for_downsampling;

            ar.labelElement("population_density_c50") & node.population_density_c50;
            ar.labelElement("population_scaling_factor") & node.population_scaling_factor;
            ar.labelElement("maternal_transmission") & node.maternal_transmission;
            ar.labelElement("vital_birth") & node.vital_birth;
            ar.labelElement("vital_birth_dependence") & (uint32_t&)node.vital_birth_dependence;
            ar.labelElement("vital_birth_time_dependence") & (uint32_t&)node.vital_birth_time_dependence;
            ar.labelElement("x_birth") & node.x_birth;

            ar.labelElement("animal_reservoir_type") & (uint32_t&)node.animal_reservoir_type;
            ar.labelElement("infectivity_scaling") & (uint32_t&)node.infectivity_scaling;
            ar.labelElement("zoonosis_rate") & node.zoonosis_rate;

            ar.labelElement("whitelist_enabled") & node.whitelist_enabled;

            ar.labelElement("infectivity_sinusoidal_forcing_amplitude") & node.infectivity_sinusoidal_forcing_amplitude;
            ar.labelElement("infectivity_sinusoidal_forcing_phase") & node.infectivity_sinusoidal_forcing_phase;
            ar.labelElement("infectivity_boxcar_forcing_amplitude") & node.infectivity_boxcar_forcing_amplitude;
            ar.labelElement("infectivity_boxcar_start_time") & node.infectivity_boxcar_start_time;
            ar.labelElement("infectivity_boxcar_end_time") & node.infectivity_boxcar_end_time;
            ar.labelElement("birth_rate_sinusoidal_forcing_amplitude") & node.birth_rate_sinusoidal_forcing_amplitude;
            ar.labelElement("birth_rate_sinusoidal_forcing_phase") & node.birth_rate_sinusoidal_forcing_phase;
            ar.labelElement("birth_rate_boxcar_forcing_amplitude") & node.birth_rate_boxcar_forcing_amplitude;
            ar.labelElement("birth_rate_boxcar_start_time") & node.birth_rate_boxcar_start_time;
            ar.labelElement("birth_rate_boxcar_end_time") & node.birth_rate_boxcar_end_time;
        }

        if ((node.serializationMask & SerializationFlags::Properties) != 0) {
            ar.labelElement("_latitude") & node._latitude;
            ar.labelElement("_longitude") & node._longitude;
// clorton          ar.labelElement("distribs") & node.distribs;
            ar.labelElement("suid_data") & node.suid.data;
            ar.labelElement("urban") & node.urban;
            ar.labelElement("birthrate") & node.birthrate;
            ar.labelElement("Above_Poverty") & node.Above_Poverty;
            ar.labelElement("family_waiting_to_migrate") & node.family_waiting_to_migrate;
            ar.labelElement("family_migration_destination") & node.family_migration_destination.data;
            ar.labelElement("family_migration_type") & (uint32_t&)node.family_migration_type;
            ar.labelElement("family_time_until_trip") & node.family_time_until_trip;
            ar.labelElement("family_time_at_destination") & node.family_time_at_destination;
            ar.labelElement("family_is_destination_new_home") & node.family_is_destination_new_home;
            ar.labelElement("Ind_Sample_Rate") & node.Ind_Sample_Rate;
// clorton          ar.labelElement("transmissionGroups") & node.transmissionGroups;
            ar.labelElement("susceptibility_dynamic_scaling") & node.susceptibility_dynamic_scaling;
// clorton          ar.labelElement("localWeather") & node.localWeather;
// clorton          ar.labelElement("migration_info") & node.migration_info;
// clorton          ar.labelElement("demographics") & node.demographics;
// clorton          ar.labelElement("demographic_distributions") & node.demographic_distributions;
            ar.labelElement("externalId") & node.externalId;
// clorton          ar.labelElement("event_context_host") & node.event_context_host;
            ar.labelElement("statPop") & node.statPop;
            ar.labelElement("Infected") & node.Infected;
            ar.labelElement("Births") & node.Births;
            ar.labelElement("Disease_Deaths") & node.Disease_Deaths;
            ar.labelElement("new_infections") & node.new_infections;
            ar.labelElement("new_reportedinfections") & node.new_reportedinfections;
            ar.labelElement("Cumulative_Infections") & node.Cumulative_Infections;
            ar.labelElement("Cumulative_Reported_Infections") & node.Cumulative_Reported_Infections;
            ar.labelElement("Campaign_Cost") & node.Campaign_Cost;
// clorton          ar.labelElement("Possible_Mothers") & node.Possible_Mothers;
            ar.labelElement("mean_age_infection") & node.mean_age_infection;
            ar.labelElement("newInfectedPeopleAgeProduct") & node.newInfectedPeopleAgeProduct;
// clorton          ar.labelElement("infected_people_prior") & node.infected_people_prior;
// clorton          ar.labelElement("infected_age_people_prior") & node.infected_age_people_prior;
            ar.labelElement("infectionrate") & node.infectionrate;
            ar.labelElement("mInfectivity") & node.mInfectivity;
            ar.labelElement("prob_maternal_transmission") & node.prob_maternal_transmission;
            ar.labelElement("immunity_dist_type") & (uint32_t&)node.immunity_dist_type;
            ar.labelElement("immunity_dist1") & node.immunity_dist1;
            ar.labelElement("immunity_dist2") & node.immunity_dist2;
            ar.labelElement("risk_dist_type") & (uint32_t&)node.risk_dist_type;
            ar.labelElement("risk_dist1") & node.risk_dist1;
            ar.labelElement("risk_dist2") & node.risk_dist2;
            ar.labelElement("migration_dist_type") & (uint32_t&)node.migration_dist_type;
            ar.labelElement("migration_dist1") & node.migration_dist1;
            ar.labelElement("migration_dist2") & node.migration_dist2;
            ar.labelElement("routes") & node.routes;
// clorton          ar.labelElement("ipkeys_whitelist") & node.ipkeys_whitelist;
        }
    }
}

#if 0
namespace Kernel
{
    template<class Archive>
    void serialize( Archive &ar, Node& node, const unsigned int v )
    {
        ar.template register_type<Kernel::ClimateByData>();
        ar.template register_type<Kernel::ClimateKoppen>();
        ar.template register_type<Kernel::ClimateConstant>();

        ar & node.individualHumans; 
        ar & node.localWeather;
        ar & node.migration_info;
        ar & node.demographics;
        ar & node.demographic_distributions;

        // The following is the reason this can't go in the .h. Controller.cpp goes crazy! :(
        ar & node.event_context_host;

        ar & node.suid;

        // Monte Carlo and adapted sampling parameters
        ar & node.Ind_Sample_Rate;

        // ar & node.features
        ar & node._latitude;
        ar & node._longitude;
        ar & node.birthrate;
        ar & node.Time;

        //  Counters
        ar & node.Possible_Mothers;
        ar & node.Infected;
        ar & node.statPop;
        ar & node.Births;

        ar & node.Campaign_Cost;

        //ar & node.cases
        ar & node.mInfectivity;
        
        ar & routes;
    }
}
#endif
