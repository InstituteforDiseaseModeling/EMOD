
#pragma once

#include "ISupports.h" 
#include "Types.h"

namespace Kernel
{
    class IInfectionHIV;

    class ISusceptibilityHIV : public ISupports
    {
    public:
        // disease specific functions go here
        virtual float              GetCD4count() const = 0;
        virtual void               FastForward( const IInfectionHIV * const, float dt ) = 0;
        virtual void               ApplyARTOnset() = 0;
        virtual ProbabilityNumber  GetPrognosisCompletedFraction() const = 0;
        virtual void               TerminateSuppression( float days_till_death ) = 0;
        virtual float              GetDaysBetweenSymptomaticAndDeath() const = 0;
        virtual bool               IsSymptomatic() const = 0;
    };
}
