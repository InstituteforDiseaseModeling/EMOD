/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Susceptibility.h"

namespace Kernel
{
    class SusceptibilityAirborneConfig : public SusceptibilityConfig
    {
    protected:
        friend class SusceptibilityAirborne;
    };
    
    class SusceptibilityAirborne : public Susceptibility
    {
    public:
        virtual ~SusceptibilityAirborne(void);
        static SusceptibilityAirborne *CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod);

    protected:

        SusceptibilityAirborne();
        SusceptibilityAirborne(IIndividualHumanContext *context);
        virtual void Initialize(float age, float immmod, float riskmod) override;

        // additional members of airborne susceptibility
        float demographic_risk;

        DECLARE_SERIALIZABLE(SusceptibilityAirborne);
    };
}
