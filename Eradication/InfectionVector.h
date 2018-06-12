/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorContexts.h"
#include "Infection.h"

namespace Kernel
{
    class InfectionVectorConfig : public InfectionConfig
    {
    protected:
        friend class InfectionVector;
    };

    class InfectionVector : public Infection
    {
    public:
        static InfectionVector *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);
        virtual ~InfectionVector(void);

    protected:
        InfectionVector();
        InfectionVector(IIndividualHumanContext *context);
        virtual void Initialize(suids::suid _suid) override;

        DECLARE_SERIALIZABLE(InfectionVector);
    };
}
