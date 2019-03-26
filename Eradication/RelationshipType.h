/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <set>
#include <vector>

#include "EnumSupport.h"

namespace Kernel
{
    ENUM_DEFINE( RelationshipType,
                 ENUM_VALUE_SPEC( TRANSITORY, 0 )
                 ENUM_VALUE_SPEC( INFORMAL,   1 )
                 ENUM_VALUE_SPEC( MARITAL,    2 )
                 ENUM_VALUE_SPEC( COMMERCIAL, 3 )
                 ENUM_VALUE_SPEC( COUNT,      4 ) )

    std::set<std::string> GetAllowableRelationshipTypes();
    std::vector<RelationshipType::Enum> GetRelationshipTypes();
    std::vector<RelationshipType::Enum> ConvertStringsToRelationshipTypes( const std::string& rParamName,
                                                                           const std::vector<std::string>& rStrings );
}
