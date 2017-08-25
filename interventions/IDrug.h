/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ISupports.h"
#include "EnumSupport.h"

namespace Kernel
{
    ENUM_DEFINE(DrugUsageType,
        ENUM_VALUE_SPEC(SingleDose                    , 1)
        ENUM_VALUE_SPEC(FullTreatmentCourse           , 2)
        ENUM_VALUE_SPEC(Prophylaxis                   , 3)
        ENUM_VALUE_SPEC(SingleDoseWhenSymptom         , 4)
        ENUM_VALUE_SPEC(FullTreatmentWhenSymptom      , 5)
        ENUM_VALUE_SPEC(SingleDoseParasiteDetect      , 6)
        ENUM_VALUE_SPEC(FullTreatmentParasiteDetect   , 7)
        ENUM_VALUE_SPEC(SingleDoseNewDetectionTech    , 8)
        ENUM_VALUE_SPEC(FullTreatmentNewDetectionTech , 9))

    struct IIndividualHumanInterventionsContext;

    struct IDrug : ISupports
    {
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc = nullptr )       = 0;

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
