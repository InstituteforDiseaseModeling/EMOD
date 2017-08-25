/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "CajunIncludes.h"

namespace Kernel
{
    struct IDistributableIntervention ;
    struct INodeDistributableIntervention;

    // Determine if an intervention is valid and throw an exception if not.
    class InterventionValidator
    {
    public:
        // These functions verify that the intervention defined in the given rElement
        // is valid for the current simulation type.  The goal is to determine during
        // initialization that the interventions being used are appropriate for the
        // current simulation type.
        static void ValidateInterventionArray( const json::Element& rElement, const std::string& rDataLocation );
        static void ValidateIntervention( const json::Element& rElement, const std::string& rDataLocation );
    };
}