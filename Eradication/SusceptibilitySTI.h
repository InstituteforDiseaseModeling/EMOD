/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Susceptibility.h"

namespace Kernel
{
    class SusceptibilitySTIConfig : public SusceptibilityConfig
    {
    protected:
        friend class SusceptibilitySTI;
    };
    
    class SusceptibilitySTI : public Susceptibility
    {
    public:
        virtual ~SusceptibilitySTI(void);
        static SusceptibilitySTI *CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod);

    protected:

        SusceptibilitySTI();
        SusceptibilitySTI(IIndividualHumanContext *context);
        virtual void Initialize(float age, float immmod, float riskmod) override;

        // additional members of airborne susceptibility
        float demographic_risk;

        DECLARE_SERIALIZABLE(SusceptibilitySTI);
    };
}
