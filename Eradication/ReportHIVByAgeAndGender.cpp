/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "Debug.h"
#include "FileSystem.h"
#include "ReportHIVByAgeAndGender.h"
#include "NodeHIV.h"
#include "SusceptibilityHIV.h"
#include "InfectionHIV.h"
#include "HIVInterventionsContainer.h"
#include "NodeLevelHealthTriggeredIV.h"
#include "ISimulation.h"

static const char* _module = "ReportHIVByAgeAndGender";

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
    {
        eventTriggerList.push_back( "DiseaseDeaths" );
        eventTriggerList.push_back( "NonDiseaseDeaths" );
        eventTriggerList.push_back( "NewInfectionEvent" );

        ZERO_ARRAY( population );
        ZERO_ARRAY( infected );
        ZERO_ARRAY( newly_infected );
        ZERO_ARRAY( on_ART );
        ZERO_ARRAY( newly_died );
        ZERO_ARRAY( newly_died_from_HIV );
        ZERO_ARRAY( tested_ever );
        ZERO_ARRAY( tested_past_year_or_onART );
    }

    bool ReportHIVByAgeAndGender::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Start_Year", &startYear, Report_HIV_ByAgeAndGender_Start_Year_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Stop_Year",  &stopYear,  Report_HIV_ByAgeAndGender_Stop_Year_DESC_TEXT,  0.0f, FLT_MAX, FLT_MAX );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret )
        {
            if( startYear < IdmDateTime::_base_year )
            {
                startYear = IdmDateTime::_base_year ;
            }
            if( startYear >= stopYear )
            {
                 throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                                                         "Report_HIV_ByAgeAndGender_Start_Year", startYear, 
                                                         "Report_HIV_ByAgeAndGender_Stop_Year", stopYear );
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
            // --- start_year == _base_year, and dt = 30, then next_report_time = 167.5 and
            // --- data would be collected at 180.  However, for the next update
            // --- next_report_time would be 350 and the update would occur at 360.
            // ------------------------------------------------------------------------
            next_report_time = DAYSPERYEAR*(startYear - IdmDateTime::_base_year) + report_hiv_half_period - dt / 2.0f ;
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

    std::string ReportHIVByAgeAndGender::GetHeader() const
    {
        std::stringstream header ;
        header << "Year"             << ", "
               << "NodeId"           << ", "
               << "Gender"           << ", "
               << "Age"              << ", "
               << "Population"       << ", "
               << "Infected"         << ", "
               << "Newly Infected"   << ", "
               << "On_ART"           << ", "
               << "Died"             << ", "
               << "Died_from_HIV"    << ", "
               << "Tested Past Year or On_ART" << ", "
               << "Tested Ever";

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

        for( int gender = 0; gender < Gender::Enum::COUNT; gender++ ) 
        {
            for( int age_bin = 0; age_bin < MAX_AGE; age_bin++ ) 
            {
                GetOutputStream() << year
                                  << ","<< nodeId 
                                  << "," << gender    // Kernel::Gender::pairs::lookup_key(gender)
                                  << "," << age_bin
                                  << "," << population[gender][age_bin]
                                  << "," << infected[gender][age_bin]
                                  << "," << newly_infected[gender][age_bin]
                                  << "," << on_ART[gender][age_bin]
                                  << "," << newly_died[gender][age_bin]
                                  << "," << newly_died_from_HIV[gender][age_bin]
                                  << "," << tested_past_year_or_onART[gender][age_bin]
                                  << "," << tested_ever[gender][age_bin]
                                  << endl;
            }
        }

        ZERO_ARRAY( population );
        ZERO_ARRAY( infected );
        ZERO_ARRAY( newly_infected );
        ZERO_ARRAY( on_ART );
        ZERO_ARRAY( newly_died );
        ZERO_ARRAY( newly_died_from_HIV );
        ZERO_ARRAY( tested_ever );
        ZERO_ARRAY( tested_past_year_or_onART );
    }

    void ReportHIVByAgeAndGender::LogIndividualData( IndividualHuman* individual )
    {
        LOG_DEBUG_F( "%s: doReport = %d\n", __FUNCTION__, doReport );

        IIndividualHumanHIV* hiv_individual = NULL;
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
        int age_bin = (int)floor( (std::min)((double) MAX_AGE-1, individual->GetAge()/(float)DAYSPERYEAR) );

        int gender = individual->GetGender() == Gender::MALE ? 0 : 1;
        bool isInfected = hiv_individual->HasHIV();
        bool isNewlyInfected = false;
        bool isOnART = hiv_individual->GetHIVInterventionsContainer()->OnArtQuery();

        bool testedPastYear = med_parent->EverTestedPastYear();
        bool testedEver = med_parent->EverTested();
        
        population[ gender ][ age_bin ] += mc_weight;

        if( isInfected )
            infected[ gender ][ age_bin ] += mc_weight;

        if( isOnART )
            on_ART[ gender ][ age_bin ] += mc_weight;

        if( testedPastYear || isOnART )
            tested_past_year_or_onART[ gender ][ age_bin ] += mc_weight;

        if( testedEver )
            tested_ever[ gender ][ age_bin ] += mc_weight;
    }

    bool ReportHIVByAgeAndGender::notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange )
    {
        LOG_DEBUG_F( "Individual %d experienced event %s\n",
                     context->GetSuid().data,
                     StateChange.c_str()
                   );

        float mc_weight = context->GetMonteCarloWeight();
        int gender = context->GetGender() == Gender::MALE ? 0 : 1;
        int age_bin = (int)floor( (std::min)((double) MAX_AGE-1, context->GetAge()/(float)DAYSPERYEAR) );

        if( StateChange == "DiseaseDeaths" ) 
        {
            newly_died[ gender ][ age_bin ] += mc_weight;
            newly_died_from_HIV[ gender ][ age_bin ] += mc_weight;
        }

        if( StateChange == "NonDiseaseDeaths" )
        {
            newly_died[ gender ][ age_bin ] += mc_weight;
        }
        else if( StateChange == "NewInfectionEvent" )
        {
            newly_infected[ gender ][ age_bin ] += mc_weight;
        }
 
        return true;
    }
}

