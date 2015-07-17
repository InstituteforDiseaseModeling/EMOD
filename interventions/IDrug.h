/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "ISupports.h"
#include "InterventionEnums.h"      // return types

namespace Kernel
{
    struct IIndividualHumanInterventionsContext;

    struct IDrug : public ISupports
    {
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc = NULL )       = 0;

        // TODO: as for vaccines, this returns values from DIFFERENT enums depending on the actual instance pointed to by the IDrug...this breaks typesafety
        virtual int GetDrugType() const = 0;

        virtual std::string         GetDrugName()      const = 0;
        virtual DrugUsageType::Enum GetDrugUsageType()       = 0;

        virtual float GetDrugReducedTransmit()      const = 0; 
        virtual float GetDrugReducedAcquire()       const = 0; 
        virtual float GetDrugCurrentConcentration() const = 0;

        virtual float GetDrugCurrentEfficacy()      const = 0;

        virtual ~IDrug() {};
    };
}
