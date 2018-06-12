/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorControlNodeTargeted.h"
#include "LarvalHabitatMultiplier.h"

namespace Kernel
{
    class ScaleLarvalHabitat : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ScaleLarvalHabitat, INodeDistributableIntervention) 

    public:
        ScaleLarvalHabitat();
        ScaleLarvalHabitat( const ScaleLarvalHabitat& master );
        virtual ~ScaleLarvalHabitat() {};

        virtual bool Configure( const Configuration * config );
        virtual void Update(float dt);

    protected:
        virtual void ApplyEffects();

        LarvalHabitatMultiplier m_LHM;
    };
}