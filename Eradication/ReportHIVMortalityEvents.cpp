/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "Debug.h"
#include "ReportHIVMortalityEvents.h"
#include "SusceptibilityHIV.h"
#include "InfectionHIV.h"
#include "IHIVInterventionsContainer.h"
#include "ISimulation.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHumanHIV.h"
#include "INodeContext.h"
#include "NodeEventContext.h"

SETUP_LOGGING( "ReportHIVMortalityEvents" )

namespace Kernel {

    struct MortalityInfo
    {
        float             death_time;
        bool              death_by_HIV;
        ExternalNodeId_t  node_id;
        int               individual_id;
        int               gender;
        float             age;
        NaturalNumber     num_relationships_at_death;
        NaturalNumber     num_lifetime_relationships;
        NaturalNumber     hiv_stage_at_death;
        float             years_since_infection; // age of infection
        NonNegativeFloat  cd4_count_current;
        NonNegativeFloat  cd4_count_first;
        NonNegativeFloat  cd4_count_last;
        NaturalNumber     days_since_CD4_blood_draw ;
        NaturalNumber     total_number_ART_initiations;
        NonNegativeFloat  total_years_on_ART;
        NonNegativeFloat  years_since_first_ART_start;
        NonNegativeFloat  years_since_latest_ART_start;
        ARTStatus::Enum   ART_status_at_death;
        std::string       intervention_state ;
        bool              ever_tested ;
        bool              ever_tested_positive ;
        bool              ever_received_CD4_result ;
        bool              ever_in_ART ;
        bool              in_ART ;

        MortalityInfo::MortalityInfo()
            : death_time(0.0)
            , death_by_HIV(false)
            , individual_id(-1)
            , gender(-1)
            , age(-1)
            , num_relationships_at_death(0)
            , num_lifetime_relationships(0)
            , hiv_stage_at_death(0)
            , years_since_infection(-1)
            , cd4_count_current(0)
            , cd4_count_first(0)
            , cd4_count_last(0)
            , days_since_CD4_blood_draw(0)
            , total_number_ART_initiations(0)
            , total_years_on_ART(0)
            , years_since_first_ART_start(0)
            , years_since_latest_ART_start(0)
            , ART_status_at_death(ARTStatus::UNDEFINED)
            , intervention_state()
            , ever_tested(false)
            , ever_tested_positive(false)
            , ever_received_CD4_result(false)
            , ever_in_ART(false)
            , in_ART(false)
        {
        }
    };


    ReportHIVMortalityEvents::ReportHIVMortalityEvents( const ISimulation* parent )
        : BaseTextReportEvents( "HIVMortality.csv" )
        , _parent( parent )
        , m_InterventionStatusKey()
    {
        eventTriggerList.push_back( EventTrigger::DiseaseDeaths );
        eventTriggerList.push_back( EventTrigger::NonDiseaseDeaths );
    }

    void ReportHIVMortalityEvents::Initialize( unsigned int nrmSize )
    {
        BaseTextReportEvents::Initialize( nrmSize );

        // has to be done if Initialize() since it is called after the demographics is read
        IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( "InterventionStatus", "", false );
        if( p_ip != nullptr )
        {
            m_InterventionStatusKey = p_ip->GetKey<IPKey>();
        }
    }

    std::string ReportHIVMortalityEvents::GetHeader() const
    {
        std::stringstream header ;
        header << "Node_ID,"
               << "id,"
               << "Death_time,"
               << "Death_was_HIV_cause,"
               << "Gender,"
               << "Age,"
               << "Num_rels_just_prior_to_death,"
               << "Num_rels_lifetime,"
               << "HIV_disease_state_just_prior_to_death,"
               << "Years_since_infection,"
               << "CD4_count_first_recorded,"
               << "CD4_count_last_recorded,"
               << "CD4_count_current,"
               << "Days_since_CD4_blood_draw,"
               << "Total_number_of_times_initiating_ART,"
               << "Total_years_on_ART,"
               << "Years_since_first_ART_initiation,"
               << "Years_since_most_recent_ART_initiation,"
               << "ART_status_just_prior_to_death,"
               << "Intervention_Status,"
               << "Ever_tested,"
               << "Ever_tested_positive,"
               << "Ever_received_CD4_result,"
               << "Ever_in_ART,"
               << "Currently_in_ART"
               ;

        return header.str();
    }

    bool
    ReportHIVMortalityEvents::notifyOnEvent(
        IIndividualHumanEventContext *context,
        const EventTrigger& trigger
    )
    {
        LOG_DEBUG_F( "Individual %d experienced event %s\n",
                     context->GetSuid().data,
                     trigger.c_str()
                   );
        IIndividualHumanHIV* hiv_individual = nullptr;
        if ( context->QueryInterface( GET_IID(IIndividualHumanHIV), (void**)&hiv_individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanEventContext", "IIndividualHumanHIV" );
        }
        release_assert( hiv_individual );
        IIndividualHumanSTI* sti_individual = nullptr;
        if (context->QueryInterface( GET_IID(IIndividualHumanSTI), (void**)&sti_individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanEventContext", "IIndividualHumanSTI" );
        }
        release_assert( sti_individual );

        MortalityInfo info;

        // --------------------------------------------------------
        // --- Assuming that the individuals in a relationship
        // --- must be in the same node.
        //release_assert( false );
        // --------------------------------------------------------
        info.node_id = context->GetNodeEventContext()->GetNodeContext()->GetExternalID();

        info.death_time = _parent->GetSimulationTime().time;
        info.death_by_HIV = false;
        {
            if( (trigger == EventTrigger::DiseaseDeaths) || (trigger == EventTrigger::NonDiseaseDeaths) )
            {
                info.individual_id = context->GetSuid().data;
                info.gender = context->GetGender();
                info.age = context->GetAge()/DAYSPERYEAR;
                info.num_relationships_at_death = sti_individual->GetNumRelationshipsAtDeath();
                info.num_lifetime_relationships = sti_individual->GetLifetimeRelationshipCount();

                // Get HIV Infection
                auto hiv_infection = hiv_individual->GetHIVInfection();
                if( hiv_infection )
                {
                    info.hiv_stage_at_death = hiv_infection->GetStage();
                    info.years_since_infection = dynamic_cast<Infection*>(hiv_infection)->GetDuration() / DAYSPERYEAR; // age of infection
                }
                else
                {
                    info.years_since_infection = 0; // age of infection
                }

                // Get HIV Suspcetbility
                auto hiv_suscept = hiv_individual->GetHIVSusceptibility();
                if( hiv_suscept )
                {
                    info.cd4_count_current = hiv_suscept->GetCD4count();
                }

                // Get IHIVMedicalHistory
                IHIVMedicalHistory * hiv_ivc = nullptr;
                release_assert( hiv_individual->GetHIVInterventionsContainer() );
                if( s_OK != hiv_individual->GetHIVInterventionsContainer()->QueryInterface(GET_IID(IHIVMedicalHistory), (void**)&hiv_ivc) )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "hiv_ivc", "IHIVMedicalHistory", "IHIVInterventionsContainer" );
                }
                if( hiv_ivc )
                {
                    info.total_number_ART_initiations = hiv_ivc->GetTotalARTInitiations();
                    info.total_years_on_ART           = hiv_ivc->GetTotalYearsOnART();
                    info.years_since_first_ART_start  = hiv_ivc->GetYearsSinceFirstARTInit();
                    info.years_since_latest_ART_start = hiv_ivc->GetYearsSinceLatestARTInit();

                    info.ever_tested              = hiv_ivc->EverTested() ;
                    info.ever_tested_positive     = hiv_ivc->EverTestedHIVPositive() ;
                    info.ever_received_CD4_result = hiv_ivc->EverReceivedCD4() ;
                    info.ever_in_ART              = hiv_ivc->EverBeenOnART() ;

                    if( info.ever_received_CD4_result )
                    {
                        info.cd4_count_first            = hiv_ivc->FirstRecordedCD4();
                        info.cd4_count_last             = hiv_ivc->LastRecordedCD4();
                        info.days_since_CD4_blood_draw  = info.death_time - hiv_ivc->TimeOfMostRecentCD4();
                    }
                }
                info.ART_status_at_death = hiv_individual->GetHIVInterventionsContainer()->GetArtStatus();
                info.in_ART              = hiv_individual->GetHIVInterventionsContainer()->OnArtQuery();

                info.intervention_state = "None";
                if( m_InterventionStatusKey.IsValid() )
                {
                    info.intervention_state = context->GetProperties()->Get( m_InterventionStatusKey ).GetValueAsString();
                }
            }
            else
            {
                LOG_DEBUG_F( "Un-handled event: %s\n", trigger.c_str() );
            }
        }

        if( trigger == EventTrigger::DiseaseDeaths )
        {
            info.death_by_HIV = true;
        }

        std::string art_status = ARTStatus::pairs::lookup_key( info.ART_status_at_death );
                 
        GetOutputStream() << info.node_id                      << ","
                          << info.individual_id                << ","
                          << info.death_time                   << ','
                          << info.death_by_HIV                 << ','
                          << info.gender                       << ','
                          << info.age                          << ','
                          << info.num_relationships_at_death   << ","
                          << info.num_lifetime_relationships   << ","
                          << info.hiv_stage_at_death           << ","
                          << info.years_since_infection        << "," // age of infection
                          << info.cd4_count_first              << ","
                          << info.cd4_count_last               << ","
                          << info.cd4_count_current            << ","
                          << info.days_since_CD4_blood_draw    << ","
                          << info.total_number_ART_initiations << ","
                          << info.total_years_on_ART           << ","
                          << info.years_since_first_ART_start  << ","
                          << info.years_since_latest_ART_start << ","
                          << art_status                        << ","
                          << info.intervention_state                << ","
                          << info.ever_tested                  << ","
                          << info.ever_tested_positive         << ","
                          << info.ever_received_CD4_result     << ","
                          << info.ever_in_ART                  << ","
                          << info.in_ART
                          << endl;
        return true;
    }
}

