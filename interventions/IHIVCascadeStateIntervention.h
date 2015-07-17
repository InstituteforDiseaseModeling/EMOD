/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include "Configure.h"

namespace Kernel
{
    // Interventions that are part of the HIV Cascade of Care should implement
    // this interface.  The Cascade of Care has the concept of having multiple
    // "cascade" states and each state can be made up of multiple interventions.
    // An individual could be in multiple states at the same time but some states
    // are not compatible.  The abort states are used to say what states the interventions
    // state is not compatible with.
    //
    // The original purpose of this interface was to provide a way for the InterventionValidator
    // to ensure that the values of an intervention's cascade and abort states are
    // all valid strings.
    struct IHIVCascadeStateIntervention : public ISupports
    {
        // Return the the cascade state of the intervention
        virtual const std::string& GetCascadeState() = 0;

        // Return the set of cascade states that would cause this intervention
        // to stop/abort.
        virtual const JsonConfigurable::tDynamicStringSet& GetAbortStates() = 0;
    };
}
