/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifndef DISABLE_AIRBORNE

#include "SusceptibilityAirborne.h"

namespace Kernel
{
    SusceptibilityAirborne *SusceptibilityAirborne::CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod)
    {
        SusceptibilityAirborne *newsusceptibility = _new_ SusceptibilityAirborne(context);
        newsusceptibility->Initialize(age, immmod, riskmod);

        return newsusceptibility;
    }

    SusceptibilityAirborne::~SusceptibilityAirborne(void) { }
    SusceptibilityAirborne::SusceptibilityAirborne() { }
    SusceptibilityAirborne::SusceptibilityAirborne(IIndividualHumanContext *context) : Susceptibility(context) { }

    void SusceptibilityAirborne::Initialize(float _age, float _immmod, float _riskmod)
    {
        Susceptibility::Initialize(_age, _immmod, _riskmod);

        // TODO: what are we doing here? 
        // initialize members of airborne susceptibility below
        demographic_risk = _riskmod; 
    }

    REGISTER_SERIALIZABLE(SusceptibilityAirborne);

    void SusceptibilityAirborne::serialize(IArchive& ar, SusceptibilityAirborne* obj)
    {
        Susceptibility::serialize(ar, obj);
        SusceptibilityAirborne& susceptibility = *obj;
        ar.labelElement("demographic_risk") & susceptibility.demographic_risk;
    }
}

#endif // DISABLE_AIRBORNE
