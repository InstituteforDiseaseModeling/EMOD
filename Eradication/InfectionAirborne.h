/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Infection.h"

namespace Kernel
{
    class InfectionAirborneConfig : public InfectionConfig
    {
    protected:
        friend class InfectionAirborne;
    };

    class InfectionAirborne : public Infection
    {
    public:
        virtual ~InfectionAirborne(void);
        static InfectionAirborne *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);

    protected:
        InfectionAirborne();
        InfectionAirborne(IIndividualHumanContext *context);

        DECLARE_SERIALIZABLE(InfectionAirborne);
    };
}
