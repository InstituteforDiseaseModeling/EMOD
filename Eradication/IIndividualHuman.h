/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "ISupports.h"
#include "suids.hpp"
#include "Common.h"
#include "Contexts.h"
#include "INodeContext.h"
#include "InterventionsContainer.h"

namespace Kernel
{
    struct IIndividualHuman : public ISupports {
        virtual suids::suid GetSuid() const = 0;
        virtual double GetAge() const = 0;
        virtual int GetGender() const = 0;
        virtual double GetMonteCarloWeight() const = 0;
        virtual bool IsInfected() const = 0;

        virtual NewInfectionState::_enum GetNewInfectionState() const = 0;
        virtual HumanStateChange GetStateChange() const = 0;

        virtual IIndividualHumanInterventionsContext* GetInterventionsContext() const = 0;
        /*virtual IIndividualHumanEventContext*         GetEventContext() = 0;*/
        /*virtual ISusceptibilityContext*               GetSusceptibilityContext() const = 0;*/

        virtual tProperties* GetProperties() = 0;
        virtual INodeContext* GetParent() const = 0;

        virtual void Update(float current_time, float dt) = 0;
    };
}
