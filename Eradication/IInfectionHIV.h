/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "HIVEnums.h"

namespace Kernel
{
    class IInfectionHIV : public ISupports
    {
    public:
        virtual float GetWHOStage() const = 0;
        virtual NaturalNumber GetViralLoad() const = 0;
        virtual float GetPrognosis() const = 0;
        virtual float GetTimeInfected() const = 0;
        virtual float GetDaysTillDeath() const = 0;
        virtual const HIVInfectionStage::Enum& GetStage() const = 0;
        virtual void SetupSuppressedDiseaseTimers() = 0;
        virtual void ApplySuppressionDropout() = 0;
        virtual void ApplySuppressionFailure() = 0;
    };
}
