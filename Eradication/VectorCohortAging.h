/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorCohort.h"

namespace Kernel
{
    struct IVectorCohortAging : ISupports
    {
        virtual float GetAge() const = 0;
        virtual void IncreaseAge( float dt ) = 0;
    };

    class VectorCohortAging : public VectorCohort, public IVectorCohortAging
    {
    public:
        DECLARE_QUERY_INTERFACE()
        virtual int32_t AddRef() override { return 1; }
        virtual int32_t Release() override { return 1; }

        virtual float GetAge() const override;
        virtual void IncreaseAge( float dt ) override;
        virtual float GetMortality( uint32_t addition ) const override;

    public:
        static VectorCohortAging *CreateCohort( float age, float progress, int32_t initial_population, const VectorMatingStructure& vms );
        virtual ~VectorCohortAging();

    protected:
        VectorCohortAging();
        VectorCohortAging(float age, float progress, int32_t initial_population, const VectorMatingStructure& _vector_genetics);
        virtual void Initialize() override;

        float age;
        // TODO - document this function/expression/calculation
        inline static float mortalityFromAge(float age) { return (0.006f * exp(0.2f * age) / (1.0f + (0.006f * 1.5f / 0.2f * (exp(0.2f * age) - 1.0f)))); }

        DECLARE_SERIALIZABLE(VectorCohortAging);
    };
}
