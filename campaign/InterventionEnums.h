/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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

    ENUM_DEFINE(EventOrConfig,
        ENUM_VALUE_SPEC(Config          , 1)
        ENUM_VALUE_SPEC(Event           , 2)) 

    // Might want to think about this name at least -- I don't think we want to go here
    ENUM_DEFINE(TBHIVInfectionType,
        ENUM_VALUE_SPEC(HIV ,   0)
        ENUM_VALUE_SPEC(TB  ,   1)
    )
}
