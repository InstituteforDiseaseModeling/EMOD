/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BoostLibWrapper.h"
#include "Individual.h"

namespace Kernel
{
    class IndividualHumanAirborneConfig : public IndividualHumanConfig
    {
    protected:
        friend class IndividualHumanAirborne;
    };

    class IndividualHumanAirborne : public IndividualHuman
    {
    public:    
        DECLARE_QUERY_INTERFACE()

        virtual ~IndividualHumanAirborne(void) { }
        static   IndividualHumanAirborne *CreateHuman(INodeContext *context, suids::suid _suid, float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0);

        // Infections and Susceptibility
        virtual void CreateSusceptibility(float=1.0, float=1.0) override;

    protected:
        IndividualHumanAirborne(suids::suid id = suids::nil_suid(), float monte_carlo_weight = 1.0f, float initial_age = 0.0f, int gender = 0);

        virtual IInfection* createInfection(suids::suid _suid) override;

        DECLARE_SERIALIZABLE(IndividualHumanAirborne);
    };
}
