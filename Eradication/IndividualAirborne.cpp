/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifndef DISABLE_AIRBORNE

#include "IndividualAirborne.h"

#include "InfectionAirborne.h"
#include "SusceptibilityAirborne.h"

static const char* _module = "IndividualHumanAirborne";

namespace Kernel
{

    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanAirborne, IndividualHuman)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanAirborne, IndividualHuman)

    IndividualHumanAirborne *IndividualHumanAirborne::CreateHuman(INodeContext *context, suids::suid id, float MCweight, float init_age, int gender, float init_poverty)
    {
        IndividualHumanAirborne *newindividual = _new_ IndividualHumanAirborne(id, MCweight, init_age, gender, init_poverty);

        newindividual->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newindividual->m_age );

        return newindividual;
    }

    void IndividualHumanAirborne::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        susceptibility = SusceptibilityAirborne::CreateSusceptibility(this, m_age, imm_mod, risk_mod);
    }

    void IndividualHumanAirborne::ReportInfectionState()
    {
        m_new_infection_state = NewInfectionState::NewInfection;
    }

    IndividualHumanAirborne::IndividualHumanAirborne(suids::suid _suid, float monte_carlo_weight, float initial_age, int gender, float initial_poverty) :
        IndividualHuman(_suid, monte_carlo_weight, initial_age, gender, initial_poverty)
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

#endif // ENABLE_TB
