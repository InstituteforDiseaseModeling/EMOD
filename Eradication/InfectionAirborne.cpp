/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifndef DISABLE_AIRBORNE

#include "InfectionAirborne.h"

namespace Kernel
{
    InfectionAirborne *InfectionAirborne::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        InfectionAirborne *newinfection = _new_ InfectionAirborne(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    InfectionAirborne::~InfectionAirborne(void) { }
    InfectionAirborne::InfectionAirborne() { }
    InfectionAirborne::InfectionAirborne(IIndividualHumanContext *context) : Infection(context) { }

    REGISTER_SERIALIZABLE(InfectionAirborne);

    void InfectionAirborne::serialize(IArchive& ar, InfectionAirborne* obj)
    {
        Infection::serialize(ar, obj);
        // InfectionAirborne doesn't add any fields to the class.
    }
}

#endif // ENABLE_TB
