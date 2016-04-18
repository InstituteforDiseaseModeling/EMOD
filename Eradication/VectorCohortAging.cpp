/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "VectorCohortAging.h"

namespace Kernel
{
    //IMPLEMENT_SERIALIZABLE(Kernel::VectorCohortAging);
 
    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorCohortAging, VectorCohort)
        HANDLE_INTERFACE(IVectorCohortAging)
    END_QUERY_INTERFACE_DERIVED(VectorCohortAging, VectorCohort)

    VectorCohortAging::VectorCohortAging()
    {
    }

    VectorCohortAging::VectorCohortAging(float _age, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics) 
        : VectorCohort(float(progress), initial_population, _vector_genetics)
        , age(_age)
    {
    }

    void VectorCohortAging::Initialize()
    {
        VectorCohort::Initialize();
    }

    VectorCohortAging *VectorCohortAging::CreateCohort(float age, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics)
    {
        VectorCohortAging *newqueue = _new_ VectorCohortAging(age, progress, initial_population, _vector_genetics);
        newqueue->Initialize();

        return newqueue;
    }

    VectorCohortAging::~VectorCohortAging()
    {
    }

    float VectorCohortAging::GetAge() const
    {
        return age;
    }

    void VectorCohortAging::IncreaseAge( float dt )
    {
        age += dt;
    }

    float VectorCohortAging::GetMortality( uint32_t addition ) const
    {
        return addition + mortalityFromAge( age );
    }

    REGISTER_SERIALIZABLE(VectorCohortAging);

    void VectorCohortAging::serialize(IArchive& ar, VectorCohortAging* obj)
    {
        VectorCohort::serialize(ar, obj);
        VectorCohortAging& cohort = *obj;
        ar.labelElement("age") & cohort.age;
    }
}
