/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IHIVInterventionsContainer.h"
#include "STIInterventionsContainer.h"

namespace Kernel
{
    struct IIndividualHumanHIV ;
    struct IDrug ;

    class HIVInterventionsContainer : public STIInterventionsContainer,
        public IHIVCampaignSemaphores,
        public IHIVMedicalHistory,
        public IHIVDrugEffects,
        public IHIVDrugEffectsApply,
        public IHIVInterventionsContainer,
        public IHIVMTCTEffects
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        HIVInterventionsContainer();
        virtual ~HIVInterventionsContainer();

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;

        // IIndividualHumanInterventionsContext
        virtual void SetContextTo(IIndividualHumanContext* context) override;

        // IHIVDrugEffectsApply
        virtual void ApplyDrugConcentrationAction( std::string , float current_concentration ) override;

        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate ) override;
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate ) override;
        virtual void ApplyDrugInactivationRateEffect( float rate ) override;
        virtual void ApplyDrugClearanceRateEffect( float rate ) override;

        virtual void ApplyProbMaternalTransmissionModifier( const ProbabilityNumber &probReduction ) override;

        virtual void GoOnART( bool viralSupression, float daysToAchieveSuppression ) override;
        virtual void GoOffART() override;

        // IHIVMedicalHistory
        // updates medical chart
        virtual void OnTestForHIV(bool test_result) override;
        virtual void OnReceivedTestResultForHIV(bool test_result) override;
        virtual void OnStageForART(bool stagedForART) override;
        virtual void OnAssessWHOStage(float WHOStage) override;
        virtual void OnTestCD4(float CD4count) override;
        virtual void OnBeginART() override;
        //
        // queries medical chart
        virtual bool EverTested() const override;
        virtual bool EverTestedPastYear() const override;
        virtual bool EverTestedHIVPositive() const override;
        virtual bool EverStaged() const override;
        virtual bool EverStagedForART() const override;
        virtual bool EverReceivedCD4() const override;
        virtual bool EverBeenOnART() const override;
        virtual float TimeOfMostRecentTest() const override;
        virtual float TimeOfMostRecentCD4() const override;
        virtual float TimeLastSeenByHealthcare() const override;
        virtual float TimeFirstStartedART() const override;
        virtual float TimeLastStartedART() const override;
        virtual float TotalTimeOnART() const override;
        virtual unsigned int NumTimesStartedART() const override;
        virtual float LowestRecordedCD4() const override;
        virtual float FirstRecordedCD4() const override;
        virtual float LastRecordedCD4() const override;
        virtual float LastRecordedWHOStage() const override;
        virtual ReceivedTestResultsType::Enum ReceivedTestResultForHIV() const override;

        // IHIVCampaignSemaphores
        virtual bool SemaphoreExists( const std::string& counter ) const override;
        virtual void SemaphoreInit( const std::string& counter, int value ) override;
        virtual int SemaphoreIncrement( const std::string& counter ) override;
        virtual bool SemaphoreDecrement( const std::string& counter ) override;

        virtual NaturalNumber GetTotalARTInitiations() const override;
        virtual NonNegativeFloat GetTotalYearsOnART() const override;
        virtual NonNegativeFloat GetYearsSinceFirstARTInit() const override;
        virtual NonNegativeFloat GetYearsSinceLatestARTInit() const override;

        // IHIVInterventionsContainer
        virtual bool OnArtQuery() const override;
        virtual const ARTStatus::Enum& GetArtStatus() const override;
        virtual bool ShouldReconstituteCD4() const override;
        virtual const ProbabilityNumber GetInfectivitySuppression() const override;
        virtual float GetDurationSinceLastStartingART() const override;
        virtual const ProbabilityNumber& GetProbMaternalTransmissionModifier() const override;
        virtual void BroadcastNewHIVInfection() override;

        // Update parameters in this container before the infections are updated
        virtual void InfectiousLoopUpdate( float dt ) override;

        // IHIVDrugEffects
        virtual float GetDrugInactivationRate() override;
        virtual float GetDrugClearanceRate() override;

    protected:

        //virtual void PropagateContextToDependents(); // pass context to interventions if they need it

        float HIV_drug_inactivation_rate;
        float HIV_drug_clearance_rate;

        // ART-related
        ARTStatus::Enum ART_status;
        float full_suppression_timer;
        float days_to_achieve_suppression;
        float days_since_most_recent_ART_start;
        float m_suppression_failure_timer;
        IIndividualHumanHIV * hiv_parent;
        ProbabilityNumber maternal_transmission_suppression;

        // campaign semaphores
        std::map<std::string, int> campaign_semaphores;

        // medical chart
        bool ever_tested_HIV_positive;
        bool ever_tested;
        bool ever_received_CD4;
        bool ever_staged_for_ART;
        bool ever_staged;
        bool ever_been_on_ART;
        float time_of_most_recent_test;
        float time_of_most_recent_CD4;
        float time_last_seen_by_healthcare;
        float time_first_started_ART ;
        float time_last_started_ART ;
        float total_time_on_ART ;
        float last_recorded_WHO_stage;
        float lowest_recorded_CD4;
        float first_recorded_CD4;
        float last_recorded_CD4;        // replaces cd4_at_last_ART_monitoring_visit
        unsigned int num_times_started_ART ;
        ReceivedTestResultsType::Enum received_HIV_test_results ;

        DECLARE_SERIALIZABLE(HIVInterventionsContainer);
    };
}
