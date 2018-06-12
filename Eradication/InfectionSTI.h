/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Infection.h"

namespace Kernel
{
    class InfectionSTIConfig : public InfectionConfig
    {
    protected:
        friend class InfectionSTI;
    };

    class InfectionSTI : public Infection
    {
    public:
        virtual ~InfectionSTI(void);
        static InfectionSTI *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);
        virtual float GetInfectiousness() const override;
        virtual void Update(float dt, ISusceptibilityContext* immunity = nullptr) override;

    protected:
        InfectionSTI();
        InfectionSTI(IIndividualHumanContext *context);

        DECLARE_SERIALIZABLE(InfectionSTI);
    };
}
