/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Types.h"
#include "ISupports.h"
#include "HIVEnums.h"

namespace Kernel
{
    #define INACTIVE_DURATION (-1.0f)

    struct IHIVIntervention: public ISupports
    {};

    struct IHIVDrugEffects : ISupports
    {
        //virtual HIVDrugTypeParameters::tHIVDTPMap& GetHIVdtParams() = 0;
        virtual float GetDrugInactivationRate() = 0;
        virtual float GetDrugClearanceRate() = 0;
        virtual ~IHIVDrugEffects() { }
    };


    // interface for HIV medical chart queries and updates
    struct IHIVMedicalHistory : public ISupports
    {
        // updates medical chart
        virtual void OnTestForHIV(bool test_result) = 0;
        virtual void OnReceivedTestResultForHIV(bool test_result) = 0;
        virtual void OnStageForART(bool stagedForART) = 0;
        virtual void OnAssessWHOStage(float WHOStage) = 0;
        virtual void OnTestCD4(float CD4count) = 0;
        virtual void OnBeginART() = 0;

        // queries medical chart
        virtual bool EverTested() const = 0;
        virtual bool EverTestedPastYear() const = 0;
        virtual bool EverTestedHIVPositive() const = 0;
        virtual bool EverStaged() const = 0;
        virtual bool EverStagedForART() const = 0;
        virtual bool EverReceivedCD4() const = 0;
        virtual bool EverBeenOnART() const = 0;
        virtual float TimeOfMostRecentTest() const = 0;
        virtual float TimeOfMostRecentCD4() const = 0;
        virtual float TimeLastSeenByHealthcare() const = 0;
        virtual float TimeFirstStartedART() const = 0 ;
        virtual float TimeLastStartedART() const = 0 ;
        virtual float TotalTimeOnART() const = 0 ;
        virtual unsigned int NumTimesStartedART() const = 0 ;
        virtual float LastRecordedWHOStage() const = 0;
        virtual float LowestRecordedCD4() const = 0;
        virtual float FirstRecordedCD4() const = 0;
        virtual float LastRecordedCD4() const = 0;
        virtual ReceivedTestResultsType::Enum ReceivedTestResultForHIV() const = 0;

        virtual NaturalNumber GetTotalARTInitiations() const = 0;
        virtual NonNegativeFloat GetTotalYearsOnART() const = 0;
        virtual NonNegativeFloat GetYearsSinceFirstARTInit() const = 0;
        virtual NonNegativeFloat GetYearsSinceLatestARTInit() const = 0;
    };

    // interface for campaign semaphores
    struct IHIVCampaignSemaphores : public ISupports
    {
        virtual bool SemaphoreExists( const std::string& counter) const = 0;
        virtual void SemaphoreInit( const std::string& counter, int value) = 0;
        virtual int SemaphoreIncrement( const std::string& counter ) = 0;
        virtual bool SemaphoreDecrement( const std::string& counter ) = 0;
    };

    struct IHIVDrugEffectsApply : public ISupports
    {
        virtual void ApplyDrugConcentrationAction( std::string , float current_concentration ) = 0;

        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate ) = 0;
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate ) = 0;
        virtual void ApplyDrugInactivationRateEffect( float rate ) = 0;
        virtual void ApplyDrugClearanceRateEffect( float rate ) = 0;

        virtual void ApplyProbMaternalTransmissionModifier( const ProbabilityNumber &probReduction ) = 0;

        virtual void GoOnART( bool viralSupression, float daysToAchieveSuppression ) = 0;
        virtual void GoOffART() = 0;
    };

    class IHIVMTCTEffects : public ISupports
    {
        public:
        virtual void ApplyProbMaternalTransmissionModifier( const ProbabilityNumber &probReduction ) = 0;
        virtual const ProbabilityNumber& GetProbMaternalTransmissionModifier() const = 0;
    };

    struct IHIVInterventionsContainer : public ISupports
    {
        virtual bool OnArtQuery() const = 0;
        virtual const ARTStatus::Enum& GetArtStatus() const = 0; // for reporting
        virtual bool ShouldReconstituteCD4() const = 0;
        virtual const ProbabilityNumber GetInfectivitySuppression() const = 0;
        virtual float GetDurationSinceLastStartingART() const = 0;
        virtual const ProbabilityNumber& GetProbMaternalTransmissionModifier() const = 0;
        virtual void BroadcastNewHIVInfection() = 0;
    };
}
