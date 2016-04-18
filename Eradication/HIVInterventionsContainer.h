/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
        public IHIVCascadeOfCare,
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
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance);
        virtual bool GiveIntervention( IDistributableIntervention * pIV );

        // IIndividualHumanInterventionsContext
        virtual void SetContextTo(IIndividualHumanContext* context);

        // IHIVDrugEffectsApply
        virtual void ApplyDrugConcentrationAction( std::string , float current_concentration );

        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate );
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate );
        virtual void ApplyDrugInactivationRateEffect( float rate );
        virtual void ApplyDrugClearanceRateEffect( float rate );

        virtual void ApplyProbMaternalTransmissionModifier( const ProbabilityNumber &probReduction );

        virtual void GoOnART( bool viralSupression, float daysToAchieveSuppression );
        virtual void GoOffART();
        virtual bool OnPreART() const;

        // IHIVCascadeOfCare
        virtual std::string getCascadeState() const;
        virtual void setCascadeState(std::string state);
        
        // IHIVMedicalHistory
        // updates medical chart
        virtual void OnTestForHIV(bool test_result);
        virtual void OnReceivedTestResultForHIV(bool test_result);
        virtual void OnStageForART(bool stagedForART);
        virtual void OnAssessWHOStage(float WHOStage);
        virtual void OnTestCD4(float CD4count);
        virtual void OnBeginART();
        virtual void OnBeginPreART();
        virtual void OnEndPreART();
        //
        // queries medical chart
        virtual bool EverTested() const;
        virtual bool EverTestedPastYear() const;
        virtual bool EverTestedHIVPositive() const;
        virtual bool EverStaged() const;
        virtual bool EverStagedForART() const;
        virtual bool EverReceivedCD4() const;
        virtual bool EverBeenOnPreART() const;
        virtual bool EverBeenOnART() const;
        virtual float TimeOfMostRecentTest() const;
        virtual float TimeOfMostRecentCD4() const;
        virtual float TimeLastSeenByHealthcare() const;
        virtual float TimeFirstStartedART() const ;
        virtual float TimeLastStartedART() const ;
        virtual float TotalTimeOnART() const ;
        virtual unsigned int NumTimesStartedART() const ;
        virtual float LowestRecordedCD4() const;
        virtual float FirstRecordedCD4() const;
        virtual float LastRecordedCD4() const;
        virtual float LastRecordedWHOStage() const;
        virtual ReceivedTestResultsType::Enum ReceivedTestResultForHIV() const;

        // IHIVCampaignSemaphores
        virtual bool SemaphoreExists(std::string counter) const;
        virtual void SemaphoreInit(std::string counter, int value);
        virtual int SemaphoreIncrement(std::string counter);
        virtual bool SemaphoreDecrement(std::string counter);

        virtual NaturalNumber GetTotalARTInitiations() const;
        virtual NonNegativeFloat GetTotalYearsOnART() const;
        virtual NonNegativeFloat GetYearsSinceFirstARTInit() const;
        virtual NonNegativeFloat GetYearsSinceLatestARTInit() const;

        // IHIVInterventionsContainer
        virtual bool OnArtQuery() const;
        virtual const ARTStatus::Enum& GetArtStatus() const;
        virtual bool ShouldReconstituteCD4() const;
        virtual const ProbabilityNumber GetInfectivitySuppression() const;
        virtual float GetDurationSinceLastStartingART() const;
        virtual const ProbabilityNumber& GetProbMaternalTransmissionModifier() const;

        virtual void Update(float dt); // hook to update interventions if they need it

        // IHIVDrugEffects
        virtual float GetDrugInactivationRate();
        virtual float GetDrugClearanceRate();

    protected:

        //virtual void PropagateContextToDependents(); // pass context to interventions if they need it
        void GiveDrug(IDrug* drug);

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

        // cascade of care
        string cascade_state;   // state specified in Campaign.json.  used for abort states and exit states

        // campaign semaphores
        std::map<std::string, int> campaign_semaphores;

        // medical chart
        bool on_PreART;
        bool ever_tested_HIV_positive;
        bool ever_tested;
        bool ever_received_CD4;
        bool ever_staged_for_ART;
        bool ever_staged;
        bool ever_been_on_PreART;
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
