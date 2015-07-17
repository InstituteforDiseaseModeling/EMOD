/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <map>
#include "EnumSupport.h"

namespace Kernel
{
    ENUM_DEFINE(SpaceSprayTarget,
        ENUM_VALUE_SPEC(SpaceSpray_FemalesOnly       , 11)
        ENUM_VALUE_SPEC(SpaceSpray_MalesOnly         , 12)
        ENUM_VALUE_SPEC(SpaceSpray_FemalesAndMales   , 13))

    ENUM_DEFINE(ArtificialDietTarget,
        //ENUM_VALUE_SPEC(AD_WithinHouse             , 20) // to be handled as individual rather than node-targeted intervention
        ENUM_VALUE_SPEC(AD_WithinVillage             , 21)
        ENUM_VALUE_SPEC(AD_OutsideVillage            , 22))
   
    ENUM_DEFINE(InterventionDurabilityProfile,
        ENUM_VALUE_SPEC(BOXDURABILITY       , 1)
        ENUM_VALUE_SPEC(DECAYDURABILITY     , 2)
        ENUM_VALUE_SPEC(BOXDECAYDURABILITY  , 3))

    ENUM_DEFINE(MalariaDrugType,
        ENUM_VALUE_SPEC(Artemisinin             , 1)
        ENUM_VALUE_SPEC(Chloroquine             , 2)
        ENUM_VALUE_SPEC(Quinine                 , 3)
        ENUM_VALUE_SPEC(SP                      , 4)
        ENUM_VALUE_SPEC(Primaquine              , 5)
        ENUM_VALUE_SPEC(Artemether_Lumefantrine , 6)
        ENUM_VALUE_SPEC(GenTransBlocking        , 7)
        ENUM_VALUE_SPEC(GenPreerythrocytic      , 8)
        ENUM_VALUE_SPEC(Tafenoquine             , 9))

    ENUM_DEFINE(MalariaChallengeType,
        ENUM_VALUE_SPEC(InfectiousBites         , 1)
        ENUM_VALUE_SPEC(Sporozoites             , 2))

    ENUM_DEFINE(TBDrugType,
        ENUM_VALUE_SPEC(DOTS                    , 1)
        ENUM_VALUE_SPEC(DOTSImproved            , 2)
        ENUM_VALUE_SPEC(EmpiricTreatment        , 3)
        ENUM_VALUE_SPEC(FirstLineCombo          , 4)
        ENUM_VALUE_SPEC(SecondLineCombo         , 5)
        ENUM_VALUE_SPEC(ThirdLineCombo          , 6)
        ENUM_VALUE_SPEC(LatentTreatment         , 7))

    ENUM_DEFINE(DrugUsageType,
        ENUM_VALUE_SPEC(SingleDose                    , 1)
        ENUM_VALUE_SPEC(FullTreatmentCourse           , 2)
        ENUM_VALUE_SPEC(Prophylaxis                   , 3)
        ENUM_VALUE_SPEC(SingleDoseWhenSymptom         , 4)
        ENUM_VALUE_SPEC(FullTreatmentWhenSymptom      , 5)
        ENUM_VALUE_SPEC(SingleDoseParasiteDetect      , 6)
        ENUM_VALUE_SPEC(FullTreatmentParasiteDetect   , 7)
        ENUM_VALUE_SPEC(SingleDoseNewDetectionTech    , 8)
        ENUM_VALUE_SPEC(FullTreatmentNewDetectionTech , 9))

    ENUM_DEFINE(BednetType,
        ENUM_VALUE_SPEC(Barrier     , 1)
        ENUM_VALUE_SPEC(ITN         , 2)
        ENUM_VALUE_SPEC(LLIN        , 3)
        ENUM_VALUE_SPEC(Retreatment , 4))

    ENUM_DEFINE(SimpleVaccineType,
        ENUM_VALUE_SPEC(Generic              , 1)
        ENUM_VALUE_SPEC(TransmissionBlocking , 2)
        ENUM_VALUE_SPEC(AcquisitionBlocking  , 3)
        ENUM_VALUE_SPEC(MortalityBlocking    , 4))

    ENUM_DEFINE(PolioVaccineType,
        ENUM_VALUE_SPEC(tOPV   , 0)
        ENUM_VALUE_SPEC(bOPV   , 1)
        ENUM_VALUE_SPEC(mOPV_1 , 2)
        ENUM_VALUE_SPEC(mOPV_2 , 3)
        ENUM_VALUE_SPEC(mOPV_3 , 4)
        ENUM_VALUE_SPEC(eIPV   , 5))

    ENUM_DEFINE(PolioDrugType,
        ENUM_VALUE_SPEC(V073                    , 1)
        ENUM_VALUE_SPEC(Combo                   , 2))

        ENUM_DEFINE(ImmunoglobulinType,
        ENUM_VALUE_SPEC(StrainSpecific      , 1)
        ENUM_VALUE_SPEC(BroadlyNeutralizing , 2))

    ENUM_DEFINE(OutdoorSprayingTarget,
        ENUM_VALUE_SPEC(FemalesOnly     , 1)
        ENUM_VALUE_SPEC(MalesOnly       , 2)
        ENUM_VALUE_SPEC(FemalesAndMales , 3))

    ENUM_DEFINE(OutbreakType, 
        ENUM_VALUE_SPEC(PrevalenceIncrease , 1)
        ENUM_VALUE_SPEC(ImportCases        , 2))

    ENUM_DEFINE(GlobalVaccineType,
        ENUM_VALUE_SPEC(VACCINE_0     , 0)
        ENUM_VALUE_SPEC(VACCINE_1     , 1) 
        ENUM_VALUE_SPEC(VACCINE_TOPV  , 2)
        ENUM_VALUE_SPEC(VACCINE_BOPV  , 3) 
        ENUM_VALUE_SPEC(VACCINE_MOPV1 , 4) 
        ENUM_VALUE_SPEC(VACCINE_MOPV2 , 5) 
        ENUM_VALUE_SPEC(VACCINE_MOPV3 , 6) 
        ENUM_VALUE_SPEC(VACCINE_IPV   , 7))

    ENUM_DEFINE(TargetDemographicType,
        ENUM_VALUE_SPEC(Everyone                    , 1) 
        ENUM_VALUE_SPEC(ExplicitAgeRanges           , 2) 
        ENUM_VALUE_SPEC(ExplicitAgeRangesAndGender  , 3) 
        ENUM_VALUE_SPEC(ExplicitGender              , 4) 
        ENUM_VALUE_SPEC(PossibleMothers             , 5) 
        ENUM_VALUE_SPEC(ArrivingAirTravellers       , 6)
        ENUM_VALUE_SPEC(DepartingAirTravellers      , 7)
        ENUM_VALUE_SPEC(ArrivingRoadTravellers      , 8)
        ENUM_VALUE_SPEC(DepartingRoadTravellers     , 9)
        ENUM_VALUE_SPEC(AllArrivingTravellers       , 10)
        ENUM_VALUE_SPEC(AllDepartingTravellers      , 11)
        ENUM_VALUE_SPEC(ExplicitDiseaseState        , 12))

    ENUM_DEFINE(TargetGroupType,
        ENUM_VALUE_SPEC(Everyone                , 1) 
        ENUM_VALUE_SPEC(Infected                , 2) 
        ENUM_VALUE_SPEC(ActiveInfection         , 3) 
        ENUM_VALUE_SPEC(LatentInfection         , 4) 
        ENUM_VALUE_SPEC(MDR                     , 5)
        ENUM_VALUE_SPEC(TreatmentNaive          , 6)
        ENUM_VALUE_SPEC(HasFailedTreatment      , 7)
        ENUM_VALUE_SPEC(HIVNegative             , 8)
        ENUM_VALUE_SPEC(ActiveHadTreatment      , 9))
        
    ENUM_DEFINE(TargetGender,
        ENUM_VALUE_SPEC(All     , 0)
        ENUM_VALUE_SPEC(Male    , 1)
        ENUM_VALUE_SPEC(Female  , 2))

    ENUM_DEFINE(InfectionType,
        ENUM_VALUE_SPEC(TB, 0)
        ENUM_VALUE_SPEC(HIV, 1))

    ENUM_DEFINE(PolygonFormatType,
        ENUM_VALUE_SPEC(SHAPE      , 1) 
        //ENUM_VALUE_SPEC(GEOJSON    , 2)
        )


// this is the index HIV event enums start
#define HIV_EVENT_ENUM_START                50

// macro to quickly add events indexed by HIV_*_ENUM_START #define's
#define HIV_ENUM(type, offset) ENUM_VALUE_SPEC(type##offset, HIV_##type##_ENUM_START + offset)

#define HIV_EVENTS_PER_TYPE 10
#define HIV_ENUM_START(x) HIV_EVENT_ENUM_START + HIV_EVENTS_PER_TYPE*x

// HIV event types/enum start indexes
#define HIV_HCTUptakeAtDebut_ENUM_START     HIV_ENUM_START(2)   // leave the first 20 free for reporting/basic events
#define HIV_HCTUptakePostDebut_ENUM_START   HIV_ENUM_START(3)
#define HIV_HCTTestingLoop_ENUM_START       HIV_ENUM_START(4)
#define HIV_TestingOnSymptomatic_ENUM_START HIV_ENUM_START(5)
#define HIV_TestingOnANC_ENUM_START         HIV_ENUM_START(6)
#define HIV_TestingOnChild6w_ENUM_START     HIV_ENUM_START(7)
#define HIV_ARTStaging_ENUM_START           HIV_ENUM_START(8)
#define HIV_ARTStaging1_ENUM_START           HIV_ENUM_START(9)
#define HIV_LinkingToPreART_ENUM_START      HIV_ENUM_START(10)
#define HIV_OnPreART_ENUM_START             HIV_ENUM_START(11)
#define HIV_OnPreART1_ENUM_START             HIV_ENUM_START(12)
#define HIV_LinkingToART_ENUM_START         HIV_ENUM_START(13)
#define HIV_OnART_ENUM_START                HIV_ENUM_START(14)
#define HIV_LTFU_ENUM_START                 HIV_ENUM_START(15)
#define HIV_LTFU1_ENUM_START                HIV_ENUM_START(16)
#define HIV_LostForever_ENUM_START          HIV_ENUM_START(17)
#define HIV_CD4Measured_ENUM_START          HIV_ENUM_START(18)
#define HIV_AgeMeasured_ENUM_START          HIV_ENUM_START(19)

    ENUM_DEFINE(IndividualEventTriggerType,
        ENUM_VALUE_SPEC(NoTrigger               , 0)
        ENUM_VALUE_SPEC(Births                  , 1)
        ENUM_VALUE_SPEC(EveryUpdate             , 2)
        ENUM_VALUE_SPEC(EveryTimeStep           , 3)
        ENUM_VALUE_SPEC(NewInfectionEvent       , 4)
        ENUM_VALUE_SPEC(TBActivation            , 5)
        ENUM_VALUE_SPEC(NewClinicalCase         , 6)
        ENUM_VALUE_SPEC(NewSevereCase           , 7)
        ENUM_VALUE_SPEC(DiseaseDeaths           , 8)
        ENUM_VALUE_SPEC(NonDiseaseDeaths        , 9)
        ENUM_VALUE_SPEC(TBActivationSmearPos    , 10)
        ENUM_VALUE_SPEC(TBActivationSmearNeg    , 11)
        ENUM_VALUE_SPEC(TBActivationExtrapulm   , 12)
        ENUM_VALUE_SPEC(TBActivationPostRelapse   , 13)
        ENUM_VALUE_SPEC(TBPendingRelapse          , 14)
        ENUM_VALUE_SPEC(TBActivationPresymptomatic, 15)
        ENUM_VALUE_SPEC(TestPositiveOnSmear       , 16)
        ENUM_VALUE_SPEC(ProviderOrdersTBTest      , 17)
        ENUM_VALUE_SPEC(TBTestPositive            , 18)
        ENUM_VALUE_SPEC(TBTestNegative            , 19)
        ENUM_VALUE_SPEC(TBTestDefault             , 20)
        ENUM_VALUE_SPEC(TBRestartHSB              , 21)
        ENUM_VALUE_SPEC(TBMDRTestPositive         , 22)
        ENUM_VALUE_SPEC(TBMDRTestNegative         , 23)
        ENUM_VALUE_SPEC(TBMDRTestDefault          , 24)
        ENUM_VALUE_SPEC(TBFailedDrugRegimen       , 25)
        ENUM_VALUE_SPEC(TBRelapseAfterDrugRegimen , 26)
        ENUM_VALUE_SPEC(TBStartDrugRegimen        , 27)
        ENUM_VALUE_SPEC(TBStopDrugRegimen         , 28)
        ENUM_VALUE_SPEC(PropertyChange            , 29)

        ENUM_VALUE_SPEC(STIDebut	            , 30)
        
        ENUM_VALUE_SPEC(StartedART	            , 31)
        ENUM_VALUE_SPEC(StoppedART	            , 32)

        ENUM_VALUE_SPEC(CascadeStateAborted	    , 33)
        
        ENUM_VALUE_SPEC(HIVNewlyDiagnosed	    , 34)
        
        ENUM_VALUE_SPEC(GaveBirth	            , 35)
        ENUM_VALUE_SPEC(Pregnant	            , 36)
        

        // HIV event triggers (KTO: group into reporters and cascade entry triggers) (max 20, see above)
        ENUM_VALUE_SPEC(HIVNeedsHIVTest	            , HIV_EVENT_ENUM_START + 0)
        ENUM_VALUE_SPEC(HIVPositiveHIVTest	        , HIV_EVENT_ENUM_START + 1)
        ENUM_VALUE_SPEC(HIVNegativeHIVTest	        , HIV_EVENT_ENUM_START + 2)
        ENUM_VALUE_SPEC(HIVDiagnosedEligibleForART	, HIV_EVENT_ENUM_START + 3)
        ENUM_VALUE_SPEC(HIVSymptomatic	            , HIV_EVENT_ENUM_START + 4)
        ENUM_VALUE_SPEC(HIVLinkToPreART	            , HIV_EVENT_ENUM_START + 5)
        ENUM_VALUE_SPEC(HIVPreARTToART	            , HIV_EVENT_ENUM_START + 6)
        ENUM_VALUE_SPEC(HIVNonPreARTToART           , HIV_EVENT_ENUM_START + 7)
        ENUM_VALUE_SPEC(TwelveWeeksPregnant         , HIV_EVENT_ENUM_START + 8)
        ENUM_VALUE_SPEC(FourteenWeeksPregnant   	, HIV_EVENT_ENUM_START + 9)
        ENUM_VALUE_SPEC(SixWeeksOld	                , HIV_EVENT_ENUM_START + 10)
        ENUM_VALUE_SPEC(EighteenMonthsOld	        , HIV_EVENT_ENUM_START + 11)
        ENUM_VALUE_SPEC(HIVNeedsMaleCircumcision    , HIV_EVENT_ENUM_START + 12)

        // HCTUptakeAtDebut state triggers (max 10 per type, see above)
        HIV_ENUM(HCTUptakeAtDebut, 0)
        HIV_ENUM(HCTUptakeAtDebut, 1)
        HIV_ENUM(HCTUptakeAtDebut, 2)
        HIV_ENUM(HCTUptakeAtDebut, 3)
        HIV_ENUM(HCTUptakeAtDebut, 4)
        HIV_ENUM(HCTUptakeAtDebut, 5)
        HIV_ENUM(HCTUptakeAtDebut, 6)
        HIV_ENUM(HCTUptakeAtDebut, 7)
        HIV_ENUM(HCTUptakeAtDebut, 8)
        HIV_ENUM(HCTUptakeAtDebut, 9)

        // HCTUptakePostDebut state triggers (max 10 per type, see above)
        HIV_ENUM(HCTUptakePostDebut, 0)
        HIV_ENUM(HCTUptakePostDebut, 1)
        HIV_ENUM(HCTUptakePostDebut, 2)
        HIV_ENUM(HCTUptakePostDebut, 3)
        HIV_ENUM(HCTUptakePostDebut, 4)
        HIV_ENUM(HCTUptakePostDebut, 5)
        HIV_ENUM(HCTUptakePostDebut, 6)
        HIV_ENUM(HCTUptakePostDebut, 7)
        HIV_ENUM(HCTUptakePostDebut, 8)
        HIV_ENUM(HCTUptakePostDebut, 9)

        // HCTTestingLoop state triggers (max 10 per type, see above)
        HIV_ENUM(HCTTestingLoop, 0)
        HIV_ENUM(HCTTestingLoop, 1)
        HIV_ENUM(HCTTestingLoop, 2)
        HIV_ENUM(HCTTestingLoop, 3)
        HIV_ENUM(HCTTestingLoop, 4)
        HIV_ENUM(HCTTestingLoop, 5)
        HIV_ENUM(HCTTestingLoop, 6)
        HIV_ENUM(HCTTestingLoop, 7)
        HIV_ENUM(HCTTestingLoop, 8)
        HIV_ENUM(HCTTestingLoop, 9)

        // TestingOnSymptomatic state triggers (max 10 per type, see above)
        HIV_ENUM(TestingOnSymptomatic, 0)
        HIV_ENUM(TestingOnSymptomatic, 1)
        HIV_ENUM(TestingOnSymptomatic, 2)
        HIV_ENUM(TestingOnSymptomatic, 3)
        HIV_ENUM(TestingOnSymptomatic, 4)
        HIV_ENUM(TestingOnSymptomatic, 5)
        HIV_ENUM(TestingOnSymptomatic, 6)
        HIV_ENUM(TestingOnSymptomatic, 7)
        HIV_ENUM(TestingOnSymptomatic, 8)
        HIV_ENUM(TestingOnSymptomatic, 9)

        // TestingOnANC state triggers (max 10 per type, see above)
        HIV_ENUM(TestingOnANC, 0)
        HIV_ENUM(TestingOnANC, 1)
        HIV_ENUM(TestingOnANC, 2)
        HIV_ENUM(TestingOnANC, 3)
        HIV_ENUM(TestingOnANC, 4)
        HIV_ENUM(TestingOnANC, 5)
        HIV_ENUM(TestingOnANC, 6)
        HIV_ENUM(TestingOnANC, 7)
        HIV_ENUM(TestingOnANC, 8)
        HIV_ENUM(TestingOnANC, 9)

        // TestingOnChild6w state triggers (max 10 per type, see above)
        HIV_ENUM(TestingOnChild6w, 0)
        HIV_ENUM(TestingOnChild6w, 1)
        HIV_ENUM(TestingOnChild6w, 2)
        HIV_ENUM(TestingOnChild6w, 3)
        HIV_ENUM(TestingOnChild6w, 4)
        HIV_ENUM(TestingOnChild6w, 5)
        HIV_ENUM(TestingOnChild6w, 6)
        HIV_ENUM(TestingOnChild6w, 7)
        HIV_ENUM(TestingOnChild6w, 8)
        HIV_ENUM(TestingOnChild6w, 9)


        // ARTStaging state triggers (max 10 per type, see above)
        HIV_ENUM(ARTStaging, 0)
        HIV_ENUM(ARTStaging, 1)
        HIV_ENUM(ARTStaging, 2)
        HIV_ENUM(ARTStaging, 3)
        HIV_ENUM(ARTStaging, 4)
        HIV_ENUM(ARTStaging, 5)
        HIV_ENUM(ARTStaging, 6)
        HIV_ENUM(ARTStaging, 7)
        HIV_ENUM(ARTStaging, 8)
        HIV_ENUM(ARTStaging, 9)

        // ARTStaging1 state triggers (max 10 per type, see above)
        HIV_ENUM(ARTStaging1, 0)
        HIV_ENUM(ARTStaging1, 1)
        HIV_ENUM(ARTStaging1, 2)
        HIV_ENUM(ARTStaging1, 3)
        HIV_ENUM(ARTStaging1, 4)
        HIV_ENUM(ARTStaging1, 5)
        HIV_ENUM(ARTStaging1, 6)
        HIV_ENUM(ARTStaging1, 7)
        HIV_ENUM(ARTStaging1, 8)
        HIV_ENUM(ARTStaging1, 9)

        // LinkingToPreART state triggers (max 10 per type, see above)
        HIV_ENUM(LinkingToPreART, 0)
        HIV_ENUM(LinkingToPreART, 1)
        HIV_ENUM(LinkingToPreART, 2)
        HIV_ENUM(LinkingToPreART, 3)
        HIV_ENUM(LinkingToPreART, 4)
        HIV_ENUM(LinkingToPreART, 5)
        HIV_ENUM(LinkingToPreART, 6)
        HIV_ENUM(LinkingToPreART, 7)
        HIV_ENUM(LinkingToPreART, 8)
        HIV_ENUM(LinkingToPreART, 9)

        // OnPreART state triggers (max 10 per type, see above)
        HIV_ENUM(OnPreART, 0)
        HIV_ENUM(OnPreART, 1)
        HIV_ENUM(OnPreART, 2)
        HIV_ENUM(OnPreART, 3)
        HIV_ENUM(OnPreART, 4)
        HIV_ENUM(OnPreART, 5)
        HIV_ENUM(OnPreART, 6)
        HIV_ENUM(OnPreART, 7)
        HIV_ENUM(OnPreART, 8)
        HIV_ENUM(OnPreART, 9)

        // OnPreART1 state triggers (max 10 per type, see above)
        HIV_ENUM(OnPreART1, 0)
        HIV_ENUM(OnPreART1, 1)
        HIV_ENUM(OnPreART1, 2)
        HIV_ENUM(OnPreART1, 3)
        HIV_ENUM(OnPreART1, 4)
        HIV_ENUM(OnPreART1, 5)
        HIV_ENUM(OnPreART1, 6)
        HIV_ENUM(OnPreART1, 7)
        HIV_ENUM(OnPreART1, 8)
        HIV_ENUM(OnPreART1, 9)


        // LinkingToART state triggers (max 10 per type, see above)
        HIV_ENUM(LinkingToART, 0)
        HIV_ENUM(LinkingToART, 1)
        HIV_ENUM(LinkingToART, 2)
        HIV_ENUM(LinkingToART, 3)
        HIV_ENUM(LinkingToART, 4)
        HIV_ENUM(LinkingToART, 5)
        HIV_ENUM(LinkingToART, 6)
        HIV_ENUM(LinkingToART, 7)
        HIV_ENUM(LinkingToART, 8)
        HIV_ENUM(LinkingToART, 9)

        // OnART state triggers (max 10 per type, see above)
        HIV_ENUM(OnART, 0)
        HIV_ENUM(OnART, 1)
        HIV_ENUM(OnART, 2)
        HIV_ENUM(OnART, 3)
        HIV_ENUM(OnART, 4)
        HIV_ENUM(OnART, 5)
        HIV_ENUM(OnART, 6)
        HIV_ENUM(OnART, 7)
        HIV_ENUM(OnART, 8)
        HIV_ENUM(OnART, 9)

        // LTFU state triggers (max 10 per type, see above)
        HIV_ENUM(LTFU, 0)
        HIV_ENUM(LTFU, 1)
        HIV_ENUM(LTFU, 2)
        HIV_ENUM(LTFU, 3)
        HIV_ENUM(LTFU, 4)
        HIV_ENUM(LTFU, 5)
        HIV_ENUM(LTFU, 6)
        HIV_ENUM(LTFU, 7)
        HIV_ENUM(LTFU, 8)
        HIV_ENUM(LTFU, 9)

        // LTFU state triggers (10-19 per type, see above)
        HIV_ENUM(LTFU1, 0)
        HIV_ENUM(LTFU1, 1)
        HIV_ENUM(LTFU1, 2)
        HIV_ENUM(LTFU1, 3)
        HIV_ENUM(LTFU1, 4)
        HIV_ENUM(LTFU1, 5)
        HIV_ENUM(LTFU1, 6)
        HIV_ENUM(LTFU1, 7)
        HIV_ENUM(LTFU1, 8)
        HIV_ENUM(LTFU1, 9)

        // LostForever state triggers (max 10 per type, see above)
        HIV_ENUM(LostForever, 0)
        HIV_ENUM(LostForever, 1)
        HIV_ENUM(LostForever, 2)
        HIV_ENUM(LostForever, 3)
        HIV_ENUM(LostForever, 4)
        HIV_ENUM(LostForever, 5)
        HIV_ENUM(LostForever, 6)
        HIV_ENUM(LostForever, 7)
        HIV_ENUM(LostForever, 8)
        HIV_ENUM(LostForever, 9)

        HIV_ENUM(CD4Measured, 0)
        HIV_ENUM(CD4Measured, 1)
        HIV_ENUM(CD4Measured, 2)
        HIV_ENUM(CD4Measured, 3)
        HIV_ENUM(CD4Measured, 4)
        HIV_ENUM(CD4Measured, 5)
        HIV_ENUM(CD4Measured, 6)
        HIV_ENUM(CD4Measured, 7)
        HIV_ENUM(CD4Measured, 8)
        HIV_ENUM(CD4Measured, 9)

        HIV_ENUM(AgeMeasured, 0)
        HIV_ENUM(AgeMeasured, 1)
        HIV_ENUM(AgeMeasured, 2)
        HIV_ENUM(AgeMeasured, 3)
        HIV_ENUM(AgeMeasured, 4)
        HIV_ENUM(AgeMeasured, 5)
        HIV_ENUM(AgeMeasured, 6)
        HIV_ENUM(AgeMeasured, 7)
        HIV_ENUM(AgeMeasured, 8)
        HIV_ENUM(AgeMeasured, 9)

        ENUM_VALUE_SPEC(TriggerString	        , 999)
        ENUM_VALUE_SPEC(TriggerList	            , 1000)
    )

#define NO_TRIGGER_STR ("NoTrigger")

    ENUM_DEFINE(BssTargetingType,
        ENUM_VALUE_SPEC(TargetBss               , 0)
        ENUM_VALUE_SPEC(IgnoreBss               , 1)
        ENUM_VALUE_SPEC(NeutralBss              , 2))


    ENUM_DEFINE(ScaleUpProfile,
        ENUM_VALUE_SPEC(Immediate       , 1)
        ENUM_VALUE_SPEC(Linear          , 2)
        ENUM_VALUE_SPEC(Sigmoid         , 3))

    ENUM_DEFINE(EventOrConfig,
        ENUM_VALUE_SPEC(Config          , 1)
        ENUM_VALUE_SPEC(Event           , 2))

    ENUM_DEFINE(BinaryBooleanOperator,
        ENUM_VALUE_SPEC(AND, 0)
        ENUM_VALUE_SPEC(OR,  1))

    ENUM_DEFINE(NATrueFalse,
        ENUM_VALUE_SPEC(NA, 0)
        ENUM_VALUE_SPEC(True,  1)
        ENUM_VALUE_SPEC(False, 2))

#define NATrueFalseSwitch(x,t) (x == NATrueFalse::Enum::True ? t : !t)

}

// TBD: I ripped out grace's AntiTB drug serialization solution. Needs alternate solution.
