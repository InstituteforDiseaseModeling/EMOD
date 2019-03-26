/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
        virtual std::vector<float> Generate_forward_CD4( bool ARTOnoff ) = 0;
        virtual void               FastForward( const IInfectionHIV * const, float dt ) = 0;
        virtual void               ApplyARTOnset() = 0;
        virtual ProbabilityNumber  GetPrognosisCompletedFraction() const = 0;
        virtual void               TerminateSuppression( float days_till_death ) = 0;
    };
}
