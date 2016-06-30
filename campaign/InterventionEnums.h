/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include "EnumSupport.h"

namespace Kernel
{
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

    ENUM_DEFINE(IndividualEventTriggerType,
        ENUM_VALUE_SPEC(NoTrigger                 ,  0)
        ENUM_VALUE_SPEC(Births                    ,  1)
        ENUM_VALUE_SPEC(EveryUpdate               ,  2)
        ENUM_VALUE_SPEC(EveryTimeStep             ,  3)
        ENUM_VALUE_SPEC(NewInfectionEvent         ,  4)
        ENUM_VALUE_SPEC(TBActivation              ,  5)
        ENUM_VALUE_SPEC(NewClinicalCase           ,  6)
        ENUM_VALUE_SPEC(NewSevereCase             ,  7)
        ENUM_VALUE_SPEC(DiseaseDeaths             ,  8)
        ENUM_VALUE_SPEC(NonDiseaseDeaths          ,  9)
        ENUM_VALUE_SPEC(TBActivationSmearPos      , 10)
        ENUM_VALUE_SPEC(TBActivationSmearNeg      , 11)
        ENUM_VALUE_SPEC(TBActivationExtrapulm     , 12)
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
        ENUM_VALUE_SPEC(STIDebut                  , 30)
        ENUM_VALUE_SPEC(StartedART                , 31)
        ENUM_VALUE_SPEC(StoppedART                , 32)
        ENUM_VALUE_SPEC(CascadeStateAborted       , 33)
        ENUM_VALUE_SPEC(HIVNewlyDiagnosed         , 34)
        ENUM_VALUE_SPEC(GaveBirth                 , 35)
        ENUM_VALUE_SPEC(Pregnant                  , 36)
        ENUM_VALUE_SPEC(Emigrating                , 37)
        ENUM_VALUE_SPEC(Immigrating               , 38)
        ENUM_VALUE_SPEC(HIVTestedNegative         , 39)
        ENUM_VALUE_SPEC(HIVTestedPositive         , 40)
        ENUM_VALUE_SPEC(HIVSymptomatic            , 41)
        ENUM_VALUE_SPEC(HIVPreARTToART            , 42)
        ENUM_VALUE_SPEC(HIVNonPreARTToART         , 43)
        ENUM_VALUE_SPEC(TwelveWeeksPregnant       , 44)
        ENUM_VALUE_SPEC(FourteenWeeksPregnant     , 45)
        ENUM_VALUE_SPEC(SixWeeksOld               , 46)
        ENUM_VALUE_SPEC(EighteenMonthsOld         , 47)
        ENUM_VALUE_SPEC(STIPreEmigrating          , 48)
        ENUM_VALUE_SPEC(STIPostImmigrating        , 49)
        ENUM_VALUE_SPEC(STINewInfection           , 50)
        
        ENUM_VALUE_SPEC(TriggerString          ,  999)
        ENUM_VALUE_SPEC(TriggerList            , 1000)
    )

#define NO_TRIGGER_STR ("NoTrigger")

    ENUM_DEFINE(EventOrConfig,
        ENUM_VALUE_SPEC(Config          , 1)
        ENUM_VALUE_SPEC(Event           , 2))
}
