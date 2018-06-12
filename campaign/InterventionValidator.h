/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "CajunIncludes.h"
#include "EnumSupport.h"

namespace Kernel
{
    struct IDistributableIntervention ;
    struct INodeDistributableIntervention;

    ENUM_DEFINE( InterventionTypeValidation,
        ENUM_VALUE_SPEC( UNKNOWN    , 0 ) 
        ENUM_VALUE_SPEC( INDIVIDUAL , 1 )
        ENUM_VALUE_SPEC( NODE       , 2 )
        ENUM_VALUE_SPEC( EITHER     , 3 ))

    // Determine if an intervention is valid and throw an exception if not.
    class InterventionValidator
    {
    public:
        // These functions verify that the intervention defined in the given rElement
        // is valid for the current simulation type.  The goal is to determine during
        // initialization that the interventions being used are appropriate for the
        // current simulation type.
        // The validation type is used to ensure that the intervention is the correct
        // type (individual-level or node-level) for the given situation.  It returns
        // the type contained.
        static InterventionTypeValidation::Enum ValidateInterventionArray( const std::string& rOwnerName,
                                                                           InterventionTypeValidation::Enum requiredType,
                                                                           const json::Element& rElement, 
                                                                           const std::string& rDataLocation );

        static InterventionTypeValidation::Enum ValidateIntervention( const std::string& rOwnerName,
                                                                      InterventionTypeValidation::Enum requiredType,
                                                                      const json::Element& rElement,
                                                                      const std::string& rDataLocation );
    };
}