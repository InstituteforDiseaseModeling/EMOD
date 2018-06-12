/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include <stdint.h>

#include "EnumSupport.h"

namespace Kernel
{
    ENUM_DEFINE(HIVInfectionStage, 
        ENUM_VALUE_SPEC(INITIATED                   , 0)
        ENUM_VALUE_SPEC(ACUTE                       , 1)
        ENUM_VALUE_SPEC(LATENT                      , 2)
        ENUM_VALUE_SPEC(AIDS                        , 3)
        ENUM_VALUE_SPEC(ON_ART                      , 4)
        ENUM_VALUE_SPEC(INFECTIONSTAGE_COUNT        , 5))

    ENUM_DEFINE(CD4_Model, 
        ENUM_VALUE_SPEC(CD4_MODEL_LINEAR            , 0)
        ENUM_VALUE_SPEC(CD4_MODEL_SQRT              , 1))

    ENUM_DEFINE(ARTStatus, 
        ENUM_VALUE_SPEC(UNDEFINED                         , 0)
        ENUM_VALUE_SPEC(WITHOUT_ART_UNQUALIFIED           , 1)
        ENUM_VALUE_SPEC(WITHOUT_ART_QUALIFIED_BY_BASELINE , 2)
        ENUM_VALUE_SPEC(ON_PRE_ART                        , 3)
        ENUM_VALUE_SPEC(LOST_TO_FOLLOW_UP                 , 4)
        ENUM_VALUE_SPEC(ON_ART_BUT_NOT_VL_SUPPRESSED      , 5)
        ENUM_VALUE_SPEC(ON_VL_SUPPRESSED                  , 6)
        ENUM_VALUE_SPEC(ON_BUT_FAILING                    , 7)
        ENUM_VALUE_SPEC(ON_BUT_ADHERENCE_POOR             , 8)
        ENUM_VALUE_SPEC(OFF_BY_DROPOUT                    , 9)
        ENUM_VALUE_SPEC(ARTSTATUS_COUNT                   , 10))

    ENUM_DEFINE(HIVDrugClass,
        ENUM_VALUE_SPEC(NucleosideReverseTranscriptaseInhibitor     , 1)    // Nucleoside or nucleotide RTI
        ENUM_VALUE_SPEC(NonNucleosideReverseTranscriptaseInhibitor  , 2)    // Non-nucleoside RTI
        ENUM_VALUE_SPEC(ProteaseInhibitor                           , 3)
        ENUM_VALUE_SPEC(IntegraseInhibitor                          , 4)
        ENUM_VALUE_SPEC(FusionInhibitor                             , 5)
        ENUM_VALUE_SPEC(GenericFirstLine                            , 6)
        ENUM_VALUE_SPEC(GenericSecondLine                           , 7)
        ENUM_VALUE_SPEC(PropertyDependent                           , 8)
        ENUM_VALUE_SPEC(COUNT                                       , 9))

    ENUM_DEFINE(ReverseTranscriptaseNucleosideAnalog,
        ENUM_VALUE_SPEC(Adenosine               , 1)
        ENUM_VALUE_SPEC(Guanosine               , 2)
        ENUM_VALUE_SPEC(Thymidine               , 3) 
        ENUM_VALUE_SPEC(Uridine                 , 4) 
        ENUM_VALUE_SPEC(Cytidine                , 5)
        ENUM_VALUE_SPEC(COUNT                   , 6))

    ENUM_DEFINE(ExtraRelationalFlagType,
        ENUM_VALUE_SPEC(Independent , 0)
        ENUM_VALUE_SPEC(Correlated  , 1)
        ENUM_VALUE_SPEC(COUNT       , 2))

    ENUM_DEFINE(ReceivedTestResultsType,
        ENUM_VALUE_SPEC(UNKNOWN  , 0)
        ENUM_VALUE_SPEC(POSITIVE , 1)
        ENUM_VALUE_SPEC(NEGATIVE , 2))
}
