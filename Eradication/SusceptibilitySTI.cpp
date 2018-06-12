/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "SusceptibilitySTI.h"

namespace Kernel
{
    SusceptibilitySTI *SusceptibilitySTI::CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod)
    {
        SusceptibilitySTI *newsusceptibility = _new_ SusceptibilitySTI(context);
        newsusceptibility->Initialize(age, immmod, riskmod);

        return newsusceptibility;
    }

    SusceptibilitySTI::~SusceptibilitySTI(void) { }
    SusceptibilitySTI::SusceptibilitySTI() { }
    SusceptibilitySTI::SusceptibilitySTI(IIndividualHumanContext *context) : Susceptibility(context) { }

    void SusceptibilitySTI::Initialize(float _age, float _immmod, float _riskmod)
    {
        Susceptibility::Initialize(_age, _immmod, _riskmod);

        // TODO: what are we doing here? 
        // initialize members of airborne susceptibility below
        demographic_risk = _riskmod; 
    }

    REGISTER_SERIALIZABLE(SusceptibilitySTI);

    void SusceptibilitySTI::serialize(IArchive& ar, SusceptibilitySTI* obj)
    {
        Susceptibility::serialize( ar, obj );
        SusceptibilitySTI& suscep = *obj;
        ar.labelElement("demographic_risk") & suscep.demographic_risk;
    }
}
