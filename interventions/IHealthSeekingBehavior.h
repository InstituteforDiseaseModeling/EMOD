/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "Interventions.h"

namespace Kernel
{
    struct IHealthSeekingBehavior : public ISupports
    {
        virtual void UpdateProbabilityofSeeking(float new_probability_of_seeking) = 0;
        //virtual ~IHealthSeekingBehavior() {} ; 
    };
}
    
