/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVInterventionsContainer.h"

#include "Log.h"
#include "Debug.h" // for release_assert
#include "MathFunctions.h"
#include "IIndividualHumanHIV.h"
#include "InfectionHIV.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include "SimulationConfig.h"
#include "Drugs.h"

// In this solution, the HIVInterventionsContainer, which owns all ART-specific business knowledge,
// notifies the immune system of the "ART event". Not sure I like that.
// The Immunity/Susceptibility responds to ART by boosting CD4, but it puts it in a new CD4 
// trajectory.

static const char * _module = "HIVInterventionsContainer";

namespace Kernel
{

    #define ON_ART() (  ART_status == ARTStatus::ON_ART_BUT_NOT_VL_SUPPRESSED   || \
                        ART_status == ARTStatus::ON_VL_SUPPRESSED               || \
                        ART_status == ARTStatus::ON_BUT_FAILING                 || \
                        ART_status == ARTStatus::ON_BUT_ADHERENCE_POOR )

    // DJK TODO:
    #define LIFETIME (120*365.0f)

    BEGIN_QUERY_INTERFACE_DERIVED(HIVInterventionsContainer, STIInterventionsContainer)
        HANDLE_INTERFACE(IHIVDrugEffects)
        HANDLE_INTERFACE(IHIVDrugEffectsApply)
        HANDLE_INTERFACE(IHIVInterventionsContainer)
        HANDLE_INTERFACE(IHIVMedicalHistory)
        HANDLE_INTERFACE(IHIVCascadeOfCare)
        HANDLE_INTERFACE(IHIVCampaignSemaphores)
        HANDLE_INTERFACE(ISTICoInfectionStatusChangeApply)
        HANDLE_INTERFACE(IHIVMTCTEffects)
    END_QUERY_INTERFACE_DERIVED(HIVInterventionsContainer, STIInterventionsContainer)

    HIVInterventionsContainer::HIVInterventionsContainer()
        : STIInterventionsContainer()
        , HIV_drug_inactivation_rate(0.0)
        , HIV_drug_clearance_rate(0.0)
        , ART_status(ARTStatus::WITHOUT_ART_UNQUALIFIED)
        , full_suppression_timer(0.0)
        , days_to_achieve_suppression(183.0f)
        , days_since_most_recent_ART_start(INACTIVE_DURATION)
        , m_suppression_failure_timer(INACTIVE_DURATION)
        , hiv_parent(nullptr)
        , maternal_transmission_suppression(0.0f)
        , cascade_state("")
        , campaign_semaphores()

        // medical chart - DJK shouldn't this live at the HIVindividual or "healthcare system" level?
        , on_PreART(false)
        , ever_tested_HIV_positive(false)
        , ever_tested(false)
        , ever_received_CD4(false)
        , ever_staged_for_ART(false)
        , ever_staged(false)
        , ever_been_on_PreART(false)
        , ever_been_on_ART(false)
        , time_of_most_recent_test(-1)
        , time_of_most_recent_CD4(-1)
        , time_last_seen_by_healthcare(-1)
        , time_first_started_ART(-1)
        , time_last_started_ART(-1)
        , total_time_on_ART(0)
        , last_recorded_WHO_stage(-1)
        , lowest_recorded_CD4(FLT_MAX)
        , first_recorded_CD4(-1)
        , last_recorded_CD4(-1)
        , num_times_started_ART(0)
        , received_HIV_test_results(ReceivedTestResultsType::UNKNOWN)
    {
    }

    HIVInterventionsContainer::~HIVInterventionsContainer()
    {
    }

    void HIVInterventionsContainer::SetContextTo(IIndividualHumanContext* context)
    {
        STIInterventionsContainer::SetContextTo( context );
        release_assert( parent );

        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**) &hiv_parent) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IIndividualHumanContext" );
        }
    }

    void HIVInterventionsContainer::Update(float dt)
    {
        // Delayed viral suppression on ART
        if( ART_status == ARTStatus::ON_ART_BUT_NOT_VL_SUPPRESSED )
        {
            full_suppression_timer -= dt;
            if( full_suppression_timer <=0 )
            {
                ART_status = ARTStatus::ON_VL_SUPPRESSED;
            }
        }

        // Failure of suppressive ART (IeDEA timer)
        if( m_suppression_failure_timer >= 0 )
        {
            m_suppression_failure_timer -= dt;
            if( m_suppression_failure_timer <= 0 )
            {
                LOG_DEBUG_F( "ART Failure for individual %d\n", parent->GetSuid().data );

                // Compute duration of failure period
                float prog = hiv_parent->GetHIVInfection()->GetPrognosis();
                float failing_duration = min( (InfectionHIVConfig::AIDS_duration_in_months*DAYSPERYEAR)/MONTHSPERYEAR, prog );

                ART_status = ARTStatus::ON_BUT_FAILING;
                full_suppression_timer = INACTIVE_DURATION;

                hiv_parent->GetHIVInfection()->ApplySuppressionFailure();
                hiv_parent->GetHIVSusceptibility()->TerminateSuppression( failing_duration );
            }
        }

        if( days_since_most_recent_ART_start >= 0.0f )
        {
            days_since_most_recent_ART_start += dt;
        }

        if( OnArtQuery() )
        {
            total_time_on_ART += dt ;
        }

        HIV_drug_inactivation_rate = 0.0;
        HIV_drug_clearance_rate = 0.0;

        STIInterventionsContainer::Update(dt);
    }

    // campaign semaphore interface (IIndividualHumanHIV)
    bool
    HIVInterventionsContainer::SemaphoreExists(std::string counter)
    const
    {
        return (campaign_semaphores.find(counter) != campaign_semaphores.end());
    }

    void
    HIVInterventionsContainer::SemaphoreInit(std::string counter, int value)
    {
        value = value < 0 ? 0 : value;
        campaign_semaphores[counter] = value;
    }

    int
    HIVInterventionsContainer::SemaphoreIncrement(std::string counter)
    {
        if (!SemaphoreExists(counter))
        {
            std::stringstream msg;
            msg << "Could not find semaphore = " << counter ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        campaign_semaphores[counter]++;
        return campaign_semaphores[counter];
    }

    bool
    HIVInterventionsContainer::SemaphoreDecrement(std::string counter)
    {
        if (!SemaphoreExists(counter))
        {
            std::stringstream msg;
            msg << "Could not find semaphore = " << counter ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        campaign_semaphores[counter]--;
        if (campaign_semaphores[counter] < 0)
        {
            campaign_semaphores[counter] = 0;
            return false;
        }
        return true;
    }

    void HIVInterventionsContainer::ApplyDrugVaccineReducedAcquireEffect(
        float prob
    )
    {
        drugVaccineReducedAcquire  *= (1.0f-prob);
    }

    void HIVInterventionsContainer::ApplyDrugVaccineReducedTransmitEffect(
        float prob
    )
    {
        drugVaccineReducedTransmit *= (1.0f-prob);
    }

    void HIVInterventionsContainer::ApplyDrugInactivationRateEffect(
        float rate
    )
    {
        HIV_drug_inactivation_rate += rate;
    }

    void HIVInterventionsContainer::ApplyDrugClearanceRateEffect(
        float rate
    )
    {
        HIV_drug_clearance_rate    += rate;
    }

    bool
    HIVInterventionsContainer::OnPreART()
    const
    {
        //return (ART_status == ARTStatus::ON_PRE_ART);
        //return strcmpi("OnPreART", cascade_state.c_str()) == 0;
        return on_PreART;
    }

    void
    HIVInterventionsContainer::setCascadeState(std::string state)
    {
        cascade_state = state;
    }

    std::string
    HIVInterventionsContainer::getCascadeState()
    const
    {
        return cascade_state;
    }

    void HIVInterventionsContainer::OnTestForHIV(bool test_result)
    {

        if( test_result && !ever_tested_HIV_positive)
        {
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, 
                                               "parent->GetEventContext()->GetNodeEventContext()", 
                                               "INodeTriggeredInterventionConsumer", 
                                               "INodeEventContext" );
            }

            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::HIVNewlyDiagnosed );
        }

        ever_tested = true;
        if (test_result)
        {
           ever_tested_HIV_positive = true;
        }
        float t = parent->GetEventContext()->GetNodeEventContext()->GetTime().time;
        time_of_most_recent_test = t;
        time_last_seen_by_healthcare = t;
    }

    void HIVInterventionsContainer::OnReceivedTestResultForHIV(bool test_result)
    {
        if( test_result )
            received_HIV_test_results = ReceivedTestResultsType::POSITIVE ;
        else
            received_HIV_test_results = ReceivedTestResultsType::NEGATIVE ;

        float t = parent->GetEventContext()->GetNodeEventContext()->GetTime().time;
        time_last_seen_by_healthcare = t;
    }

    void HIVInterventionsContainer::OnStageForART(bool stagedForART)
    {
        ever_staged = true;
        ever_staged_for_ART |= stagedForART;
        float t = parent->GetEventContext()->GetNodeEventContext()->GetTime().time;
        time_last_seen_by_healthcare = t;
    }

    void HIVInterventionsContainer::OnAssessWHOStage(float WHOStage)
    {
        float t = parent->GetEventContext()->GetNodeEventContext()->GetTime().time;
        time_last_seen_by_healthcare = t;
        last_recorded_WHO_stage = WHOStage;
    }

    void HIVInterventionsContainer::OnTestCD4(float cd4_value)
    {
        //ever_tested = true;
        if( !ever_received_CD4 )
        {
            ever_received_CD4 = true;
            first_recorded_CD4 = cd4_value;
        }
        last_recorded_CD4 = cd4_value;
        lowest_recorded_CD4 = min(lowest_recorded_CD4, cd4_value);
        float t = parent->GetEventContext()->GetNodeEventContext()->GetTime().time;
        //time_of_most_recent_test = t;
        time_last_seen_by_healthcare = t;
        time_of_most_recent_CD4 = t ;
    }

    void HIVInterventionsContainer::OnBeginPreART()
    {
        on_PreART = true;
        ever_been_on_PreART = true;
    }

    void HIVInterventionsContainer::OnEndPreART()
    {
        on_PreART = false;
    }

    void HIVInterventionsContainer::OnBeginART()
    {
        float t = parent->GetEventContext()->GetNodeEventContext()->GetTime().time;
        if( !ever_been_on_ART )
        {
            ever_been_on_ART = true;
            time_first_started_ART = t ;
        }
        num_times_started_ART++ ;
        time_last_started_ART = t ;
    }

    bool HIVInterventionsContainer::EverTested()
    const
    {
        return ever_tested;
    }

    bool HIVInterventionsContainer::EverTestedPastYear() const
    {
        float t = parent->GetEventContext()->GetNodeEventContext()->GetTime().time;

        return EverTested() && (t - TimeOfMostRecentTest()) <= DAYSPERYEAR;
    }

    bool HIVInterventionsContainer::EverTestedHIVPositive()
    const
    {
        return ever_tested_HIV_positive;
    }

    ReceivedTestResultsType::Enum HIVInterventionsContainer::ReceivedTestResultForHIV()
    const
    {
        return received_HIV_test_results;
    }

    bool HIVInterventionsContainer::EverStaged()
    const
    {
        return ever_staged;
    }

    bool HIVInterventionsContainer::EverStagedForART()
    const
    {
        return ever_staged_for_ART;
    }

    bool HIVInterventionsContainer::EverReceivedCD4()
    const
    {
        return ever_received_CD4;
    }

    bool HIVInterventionsContainer::EverBeenOnPreART()
    const
    {
        return ever_been_on_PreART;
    }

    bool HIVInterventionsContainer::EverBeenOnART()
    const
    {
        return ever_been_on_ART;
    }

    float HIVInterventionsContainer::TimeOfMostRecentTest()
    const
    {
        return time_of_most_recent_test;
    }

    float HIVInterventionsContainer::TimeOfMostRecentCD4()
    const
    {
        return time_of_most_recent_CD4;
    }

    float HIVInterventionsContainer::TimeLastSeenByHealthcare()
    const
    {
        return time_last_seen_by_healthcare;
    }

    float HIVInterventionsContainer::TimeFirstStartedART() const
    {
        return time_first_started_ART ;
    }

    float HIVInterventionsContainer::TimeLastStartedART() const
    {
        return time_last_started_ART ;
    }

    float HIVInterventionsContainer::TotalTimeOnART() const
    {
        return total_time_on_ART ;
    }

    unsigned int HIVInterventionsContainer::NumTimesStartedART() const
    {
        return num_times_started_ART ;
    }

    float HIVInterventionsContainer::LowestRecordedCD4()
    const
    {
        return lowest_recorded_CD4;
    }

    float HIVInterventionsContainer::FirstRecordedCD4()
    const
    {
        return first_recorded_CD4;
    }

    float HIVInterventionsContainer::LastRecordedCD4()
    const
    {
        return last_recorded_CD4;
    }

    float HIVInterventionsContainer::LastRecordedWHOStage()
    const
    {
        return last_recorded_WHO_stage;
    }

    NaturalNumber
    HIVInterventionsContainer::GetTotalARTInitiations()
    const
    {
        return num_times_started_ART;
    }

    NonNegativeFloat
    HIVInterventionsContainer::GetTotalYearsOnART()
    const
    {
        float years = 0.0f ;
        if( ever_been_on_ART )
        {
            years = total_time_on_ART / DAYSPERYEAR ;
        }
        else
        {
            LOG_DEBUG_F( "GetTotalYearsOnART for individual %d who has never been on ART.\n", parent->GetSuid().data );
        }

        return years ;
    }

    NonNegativeFloat
    HIVInterventionsContainer::GetYearsSinceFirstARTInit()
    const
    {
        float years_since_first = 0.0f ;
        if( ever_been_on_ART )
        {
            float t = parent->GetEventContext()->GetNodeEventContext()->GetTime().time;
            years_since_first = (t - time_first_started_ART) / DAYSPERYEAR ;
        }
        return years_since_first ;
    }

    NonNegativeFloat
    HIVInterventionsContainer::GetYearsSinceLatestARTInit()
    const
    {
        float years_since_last = 0.0f ;
        if( ever_been_on_ART )
        {
            float t = parent->GetEventContext()->GetNodeEventContext()->GetTime().time;
            years_since_last = (t - time_last_started_ART) / DAYSPERYEAR ;
        }
        return years_since_last ;
    }

    void HIVInterventionsContainer::GoOffART()
    {
        release_assert( hiv_parent );
        release_assert( hiv_parent->GetHIVSusceptibility() );
        if( hiv_parent->GetHIVInfection() == nullptr )
        {
            LOG_WARN_F( "Individual %d coming off ART without infection!!!\n", parent->GetSuid().data );
            return;
        }

        release_assert( hiv_parent->GetHIVInfection() );
        if( OnArtQuery() == false )
        {
            LOG_DEBUG_F( "Individual %d is not on ART, cannot dropout.\n", parent->GetSuid().data );
            return;
        }

        LOG_DEBUG_F( "EEL: Individual %d dropping off ART now\n", parent->GetSuid().data );
        ART_status = ARTStatus::OFF_BY_DROPOUT;
        full_suppression_timer = INACTIVE_DURATION;
        days_since_most_recent_ART_start = INACTIVE_DURATION;
        hiv_parent->GetHIVInfection()->ApplySuppressionDropout();

        // Make sure ART dropouts do no return to ON_BUT_FAILING
        m_suppression_failure_timer = INACTIVE_DURATION;

        INodeTriggeredInterventionConsumer* broadcaster = nullptr;
        if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, 
                                           "parent->GetEventContext()->GetNodeEventContext()", 
                                           "INodeTriggeredInterventionConsumer",
                                           "INodeEventContext" );
        }

        broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::StoppedART );
    }

    void HIVInterventionsContainer::ApplyDrugConcentrationAction( std::string , float current_concentration )
    {
        // DJK TODO:
    }

    void HIVInterventionsContainer::GoOnART( bool viral_suppression, float daysToAchieveSuppression )
    {
        INodeTriggeredInterventionConsumer* broadcaster = nullptr;
        if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent->GetEventContext()->GetNodeEventContext()",
                                           "INodeTriggeredInterventionConsumer", 
                                           "INodeEventContext" );
        }

        // NOTE: Should determine where GoOnART was called from by listening to other broadcast messages
        if( OnPreART() )
        {
            // broadcast HIVPreARTToART
            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::HIVPreARTToART );
        }
        else
        {
            // broadcast HIVPreARTToART
            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::HIVNonPreARTToART );
        }

        release_assert( hiv_parent );
        release_assert( hiv_parent->GetHIVSusceptibility() );
        if( hiv_parent->GetHIVInfection() == nullptr )
        {
            LOG_DEBUG_F( "GoOnART called for *uninfected* individual %d.  Not distributing ART!\n", parent->GetSuid().data );
            return;
        }

        release_assert( hiv_parent->GetHIVInfection() );
        if( OnArtQuery() == true )
        {
            // Don't got on ART if already on ART!
            LOG_DEBUG_F( "Individual %d is already on ART.\n", parent->GetSuid().data );
            return;
        }

        if( OnPreART() ) 
        {
            OnEndPreART();
            LOG_DEBUG_F( "Individual %d appears to be in pre-ART while starting ART, changing preART status indicator to false.\n", parent->GetSuid().data );
        }

        OnBeginART();

        if( viral_suppression ) 
        {
            hiv_parent->GetHIVSusceptibility()->ApplyARTOnset();
            hiv_parent->GetHIVInfection()->SetupSuppressedDiseaseTimers();
            ART_status = ARTStatus::ON_ART_BUT_NOT_VL_SUPPRESSED;
            full_suppression_timer = daysToAchieveSuppression;
            days_to_achieve_suppression = daysToAchieveSuppression;
        }
        else
        {
            ART_status = ARTStatus::ON_BUT_ADHERENCE_POOR;
        }

        days_since_most_recent_ART_start = 0.0f;

        broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::StartedART );
        LOG_DEBUG_F( "Individual %d is now on ART.\n", parent->GetSuid().data );

        // If not going to achieve viral suppression, stop here so as to 1) avoid computing failure and 2) skip maternal transmission mod
        if( !viral_suppression )
            return;

        float prog = hiv_parent->GetHIVInfection()->GetPrognosis();
        // Simple case (prog>11.9 months):
        // |--early(183d)--x----suppression(yrs)----x--failing(9m)--|
        // Shorter case (prog=11.9m):
        // |--early--x---failing---| ... straight from ramp-up to failing, suppression first to go
        // Even Shorter case (prog>183d && <11.9m):
        // |-early-x---failing---| ... straight from ramp-up to failing, ramp-up gets trimmed
        // Shortest case (prog<=9m):
        // |--failing--| ... straight to failing

        float failing_duration = min( (InfectionHIVConfig::AIDS_duration_in_months*DAYSPERYEAR)/MONTHSPERYEAR, prog );
        NO_LESS_THAN( failing_duration, 0.0f); // not really necessary

        float early_duration        = min( days_to_achieve_suppression, prog - failing_duration );
        float suppression_duration  = max( prog - early_duration - failing_duration, 0.0f );

        // set up a timer for ART failure
        LOG_DEBUG_F( "Individual %d getting suppression_duration (time to ART failure) of %f based on prognosis of %f (early = %f, failing = %f).\n",
                        parent->GetSuid().data,
                        suppression_duration,
                        prog,
                        early_duration,
                        failing_duration
                    );
        m_suppression_failure_timer = suppression_duration + early_duration;

    }

    const ARTStatus::Enum&
    HIVInterventionsContainer::GetArtStatus() const
    {
        return ART_status;
    }

    bool HIVInterventionsContainer::ShouldReconstituteCD4() const
    {
        return ART_status == ARTStatus::ON_ART_BUT_NOT_VL_SUPPRESSED ||
               ART_status == ARTStatus::ON_VL_SUPPRESSED;
    }

    const ProbabilityNumber
    HIVInterventionsContainer::GetInfectivitySuppression()
    const
    {
        // If not on art, 1.0f, else if ramping up, it's linear from 1.0 to 0.08, else if ramped up, 0.08f;
        ProbabilityNumber ret = 1.0f;
        float multiplier = InfectionHIVConfig::ART_viral_suppression_multiplier ;
        if( ART_status == ARTStatus::ON_ART_BUT_NOT_VL_SUPPRESSED )
        {
            if( days_to_achieve_suppression == 0.0 )
            {
                ret = multiplier;
            }
            else
            {
                float time_since_starting_ART = days_to_achieve_suppression - full_suppression_timer ;
                ret = 1.0f - ( ((1.0f - multiplier)/days_to_achieve_suppression) * (time_since_starting_ART) );
            }
        }
        else if( ART_status == ARTStatus::ON_VL_SUPPRESSED )
        {
            ret = multiplier;
        }
        LOG_DEBUG_F( "Returning viral suppression factor of %f.\n", (float) ret );
        return ret;
    }

    float
    HIVInterventionsContainer::GetDurationSinceLastStartingART()
    const
    {
        return days_since_most_recent_ART_start;
    }

    void HIVInterventionsContainer::GiveDrug(IDrug* drug)
    {
        drug->ConfigureDrugTreatment(this);
    }

    // For now, before refactoring Drugs to work in new way, just check if the intervention is a
    // Drug, and if so, add to drugs list. In future, there will be no drugs list, just interventions.
    bool HIVInterventionsContainer::GiveIntervention(
        IDistributableIntervention * pIV
    )
    {
        // NOTE: Calling this AFTER the QI/GiveDrug crashes!!! Both win and linux. Says SetContextTo suddenly became a pure virtual.
        pIV->SetContextTo( parent );
        IDrug * pDrug = nullptr;
        if( s_OK == pIV->QueryInterface(GET_IID(IDrug), (void**) &pDrug) )
        {
            LOG_DEBUG("Getting a HIV drug\n");
            GiveDrug( pDrug );
        }

        return STIInterventionsContainer::GiveIntervention( pIV );
    }

    float HIVInterventionsContainer::GetDrugInactivationRate() { return HIV_drug_inactivation_rate; }
    float HIVInterventionsContainer::GetDrugClearanceRate()    { return HIV_drug_clearance_rate; }

    bool
    HIVInterventionsContainer::OnArtQuery() const
    {
        return ON_ART();
    }

    void
    HIVInterventionsContainer::ApplyProbMaternalTransmissionModifier(
        const ProbabilityNumber &probReduction
    )
    {
        if( probReduction == 0.0f || ( probReduction > maternal_transmission_suppression ) )
        {
            maternal_transmission_suppression = probReduction;
            LOG_DEBUG_F( "ApplyProbMaternalTransmissionModifier: intervention set maternal_transmission_suppression to (%f)\n", 
                         (float) maternal_transmission_suppression );
        }
        else
        {
            LOG_DEBUG_F( "ApplyProbMaternalTransmissionModifier: attempt to update failed because new value (%f) is less than (or equal to) current value (%f)\n",
                         (float) probReduction, (float) maternal_transmission_suppression );
        }
    }

    const ProbabilityNumber&
    HIVInterventionsContainer::GetProbMaternalTransmissionModifier()
    const
    {
        return maternal_transmission_suppression;
    }

    REGISTER_SERIALIZABLE(HIVInterventionsContainer);

    void HIVInterventionsContainer::serialize(IArchive& ar, HIVInterventionsContainer* obj)
    {
        STIInterventionsContainer::serialize( ar, obj );
        HIVInterventionsContainer& container = *obj;

        ar.labelElement("HIV_drug_inactivation_rate"       ) & container.HIV_drug_inactivation_rate;
        ar.labelElement("HIV_drug_clearance_rate"          ) & container.HIV_drug_clearance_rate;
        ar.labelElement("ART_status"                       ) & (uint32_t&)container.ART_status;
        ar.labelElement("full_suppression_timer"           ) & container.full_suppression_timer;
        ar.labelElement("days_to_achieve_suppression"      ) & container.days_to_achieve_suppression;
        ar.labelElement("days_since_most_recent_ART_start" ) & container.days_since_most_recent_ART_start;
        ar.labelElement("m_suppression_failure_timer"      ) & container.m_suppression_failure_timer;
        ar.labelElement("maternal_transmission_suppression") & container.maternal_transmission_suppression;
        ar.labelElement("cascade_state"                    ) & container.cascade_state;
        ar.labelElement("campaign_semaphores"              ) & container.campaign_semaphores;
        ar.labelElement("on_PreART"                        ) & container.on_PreART;
        ar.labelElement("ever_tested_HIV_positive"         ) & container.ever_tested_HIV_positive;
        ar.labelElement("ever_tested"                      ) & container.ever_tested;
        ar.labelElement("ever_received_CD4"                ) & container.ever_received_CD4;
        ar.labelElement("ever_staged_for_ART"              ) & container.ever_staged_for_ART;
        ar.labelElement("ever_staged"                      ) & container.ever_staged;
        ar.labelElement("ever_been_on_PreART"              ) & container.ever_been_on_PreART;
        ar.labelElement("ever_been_on_ART"                 ) & container.ever_been_on_ART;
        ar.labelElement("time_of_most_recent_test"         ) & container.time_of_most_recent_test;
        ar.labelElement("time_of_most_recent_CD4"          ) & container.time_of_most_recent_CD4;
        ar.labelElement("time_last_seen_by_healthcare"     ) & container.time_last_seen_by_healthcare;
        ar.labelElement("time_first_started_ART"           ) & container.time_first_started_ART;
        ar.labelElement("time_last_started_ART"            ) & container.time_last_started_ART;
        ar.labelElement("total_time_on_ART"                ) & container.total_time_on_ART;
        ar.labelElement("last_recorded_WHO_stage"          ) & container.last_recorded_WHO_stage;
        ar.labelElement("lowest_recorded_CD4"              ) & container.lowest_recorded_CD4;
        ar.labelElement("first_recorded_CD4"               ) & container.first_recorded_CD4;
        ar.labelElement("last_recorded_CD4"                ) & container.last_recorded_CD4;
        ar.labelElement("num_times_started_ART"            ) & container.num_times_started_ART;
        ar.labelElement("received_HIV_test_results"        ) & (uint32_t&)container.received_HIV_test_results;

        //hiv_parent set in SetContextTo
    }
}
