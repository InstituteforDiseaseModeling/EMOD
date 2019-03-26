/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#if defined(ENABLE_ENVIRONMENTAL)

#include "SusceptibilityEnvironmental.h"

namespace Kernel
{
    SusceptibilityEnvironmental::SusceptibilityEnvironmental() : Susceptibility()
    {
    }

    SusceptibilityEnvironmental::SusceptibilityEnvironmental(IIndividualHumanContext *context) : Susceptibility(context)
    {
    }

    void SusceptibilityEnvironmental::Initialize(float _age, float _immmod, float _riskmod)
    {
        Susceptibility::Initialize(_age, _immmod, _riskmod);

        demographic_risk = _riskmod; 
    }

    SusceptibilityEnvironmental *SusceptibilityEnvironmental::CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod)
    {
        SusceptibilityEnvironmental *newsusceptibility = _new_ SusceptibilityEnvironmental(context);
        newsusceptibility->Initialize(age, immmod, riskmod);

        return newsusceptibility;
    }

    SusceptibilityEnvironmental::~SusceptibilityEnvironmental(void)
    {
    }

    REGISTER_SERIALIZABLE(SusceptibilityEnvironmental);

    void SusceptibilityEnvironmental::serialize(IArchive& ar, SusceptibilityEnvironmental* obj)
    {
        Susceptibility::serialize(ar, obj);
        SusceptibilityEnvironmental& susceptibility = *obj;
        ar.labelElement("demographic_risk") & susceptibility.demographic_risk;
    }
}

#endif // ENABLE_POLIO
