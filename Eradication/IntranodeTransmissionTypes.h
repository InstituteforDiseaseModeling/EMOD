/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "EnumSupport.h"

namespace Kernel
{
    ENUM_DEFINE(TransmissionRoute, 
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_ALL                             , 0)
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_CONTACT                         , 1)
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_ENVIRONMENTAL                   , 2)
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_VECTOR_TO_HUMAN                 , 10)
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR          , 11)
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR         , 12)
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_HUMAN_TO_VECTOR                 , 20)
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR          , 21)
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR         , 22)
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_INDOOR                          , 31)
        ENUM_VALUE_SPEC(TRANSMISSIONROUTE_OUTDOOR                         , 32))

    ENUM_DEFINE(TransmissionGroupType,
        ENUM_VALUE_SPEC(SimpleGroups                                      , 0)
        ENUM_VALUE_SPEC(MultiRouteGroups                                  , 1)
        ENUM_VALUE_SPEC(StrainAwareGroups                                 , 2)
        ENUM_VALUE_SPEC(RelationshipGroups                                , 3)
        ENUM_VALUE_SPEC(HumanVectorGroups                                 , 4))
}
