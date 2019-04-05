/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Infection.h"

namespace Kernel
{
    class InfectionEnvironmentalConfig : public InfectionConfig
    {
    protected:
        friend class InfectionEnvironmental;
    };

    class InfectionEnvironmental : public Infection
    {
    public:
        static InfectionEnvironmental *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);
        virtual ~InfectionEnvironmental(void);

        virtual void Update(float dt, ISusceptibilityContext* immunity = nullptr) override;
        virtual void SetParameters(IStrainIdentity* _infstrain=nullptr, int incubation_period_override = -1 ) override;

    protected:
        InfectionEnvironmental(IIndividualHumanContext *context);
        virtual void Initialize(suids::suid _suid) override;
        InfectionEnvironmental();

        DECLARE_SERIALIZABLE(InfectionEnvironmental);
    };
}
