/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "VectorCohort.h"
#include "Common.h"
#include "BoostLibWrapper.h"

namespace Kernel
{
    class IVectorCohortAging : public ISupports
    {
    public:
        virtual float GetAge() const = 0;
        virtual void IncreaseAge( float dt ) = 0;
    };

    class VectorCohortAging : public VectorCohort, public IVectorCohortAging
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        virtual float GetAge() const;
        virtual void IncreaseAge( float dt );
        virtual float GetMortality( uint32_t addition ) const;

    public:
        static VectorCohortAging *CreateCohort( float age = 0.0f, float progress = 0.0f, int32_t initial_population = 0, VectorMatingStructure = VectorMatingStructure() );
        virtual ~VectorCohortAging();

    protected:
        VectorCohortAging();
        VectorCohortAging(float age, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics);
        void Initialize();

        float age;
        // TODO - document this function/expression/calculation
        inline static float mortalityFromAge(float age) { return (0.006f * exp(0.2f * age) / (1.0f + (0.006f * 1.5f / 0.2f * (exp(0.2f * age) - 1.0f)))); }

    private:

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, VectorCohortAging& cohort, const unsigned int  file_version );
#endif
    };
}
