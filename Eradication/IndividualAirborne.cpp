/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifndef DISABLE_AIRBORNE

#include "IndividualAirborne.h"

#include "InfectionAirborne.h"
#include "SusceptibilityAirborne.h"

SETUP_LOGGING( "IndividualAirborne" )

namespace Kernel
{

    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanAirborne, IndividualHuman)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanAirborne, IndividualHuman)

    IndividualHumanAirborne *IndividualHumanAirborne::CreateHuman(INodeContext *context, suids::suid id, float MCweight, float init_age, int gender)
    {
        IndividualHumanAirborne *newindividual = _new_ IndividualHumanAirborne(id, MCweight, init_age, gender);

        newindividual->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newindividual->m_age );

        return newindividual;
    }

    void IndividualHumanAirborne::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        susceptibility = SusceptibilityAirborne::CreateSusceptibility(this, m_age, imm_mod, risk_mod);
    }

    IndividualHumanAirborne::IndividualHumanAirborne(suids::suid _suid, float monte_carlo_weight, float initial_age, int gender) :
        IndividualHuman(_suid, monte_carlo_weight, initial_age, gender)
    {
    }

    IInfection* IndividualHumanAirborne::createInfection( suids::suid _suid )
    {
        return InfectionAirborne::CreateInfection(this, _suid);
    }

    REGISTER_SERIALIZABLE(IndividualHumanAirborne);

    void IndividualHumanAirborne::serialize(IArchive& ar, IndividualHumanAirborne* obj)
    {
        IndividualHuman::serialize(ar, obj);
        // IndividualHumanAirborne doesn't have any additional fields.
    }
}

#endif // DISABLE_AIRBORNE
