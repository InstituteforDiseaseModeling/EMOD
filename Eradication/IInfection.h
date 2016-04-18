/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "ISusceptibilityContext.h"
#include "Common.h"             // InfectionStateChange
#include "StrainIdentity.h"     // TODO- Use IStrainIdentity (doesn't exist yet)
#include "Types.h"              // NonNegativeFloat

namespace Kernel
{
    struct IIndividualHumanContext;

    struct IInfection : ISerializable
    {
        virtual void Update(float, ISusceptibilityContext* = nullptr) = 0;
        virtual InfectionStateChange::_enum GetStateChange() const = 0;
        virtual float GetInfectiousness() const = 0;
        virtual float GetInfectiousnessByRoute(std::string route) const = 0;
        virtual void GetInfectiousStrainID(StrainIdentity*) = 0;
        virtual bool IsActive() const = 0;
        virtual NonNegativeFloat GetDuration() const = 0;
        virtual void SetContextTo(IIndividualHumanContext*) = 0;
        virtual void SetParameters(StrainIdentity* infstrain=nullptr, int incubation_period_override = -1 ) = 0;
        virtual void InitInfectionImmunology(ISusceptibilityContext* _immunity) = 0;

        virtual ~IInfection() {}
    };

    typedef std::list<IInfection*> infection_list_t;
}