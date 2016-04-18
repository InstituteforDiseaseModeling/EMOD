/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorCohort.h"
#include "BoostLibWrapper.h"
#include "Common.h"

namespace Kernel
{
    class VectorHabitat;

    class IVectorCohortWithHabitat : public ISupports
    {
    public:
        virtual VectorHabitat* GetHabitat() = 0;
    };

    class VectorCohortWithHabitat : public VectorCohort, public IVectorCohortWithHabitat
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorCohortWithHabitat *CreateCohort( VectorHabitat* _habitat = nullptr, float progress = 0.0f, int32_t initial_population = 0, VectorMatingStructure _vector_genetics = VectorMatingStructure() );
        virtual ~VectorCohortWithHabitat();

        virtual VectorHabitat* GetHabitat();

    protected:
        VectorHabitat* habitat;
        VectorCohortWithHabitat();
        VectorCohortWithHabitat(VectorHabitat* _habitat, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics);
        void Initialize();
    };
}
