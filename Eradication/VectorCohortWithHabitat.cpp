/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "VectorCohortWithHabitat.h"
#include "VectorHabitat.h"

namespace Kernel
{
 
    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorCohortWithHabitat, VectorCohort)
        HANDLE_INTERFACE(IVectorCohortWithHabitat)
    END_QUERY_INTERFACE_DERIVED(VectorCohortWithHabitat, VectorCohort)

    VectorCohortWithHabitat::VectorCohortWithHabitat()
        : habitat( nullptr )
        , habitat_type( (VectorHabitatType::Enum)0 )
    {
    }

    VectorCohortWithHabitat::VectorCohortWithHabitat( IVectorHabitat* _habitat, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics )
        : VectorCohort((float)progress, initial_population, _vector_genetics)
        , habitat(_habitat)
        , habitat_type( _habitat ? _habitat->GetVectorHabitatType() : (VectorHabitatType::Enum) 0 )
    {
    }

    void VectorCohortWithHabitat::Initialize()
    {
        VectorCohort::Initialize();
    }

    VectorCohortWithHabitat *VectorCohortWithHabitat::CreateCohort( IVectorHabitat* _habitat, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics )
    {
        VectorCohortWithHabitat *newqueue = _new_ VectorCohortWithHabitat(_habitat, progress, initial_population, _vector_genetics);
        newqueue->Initialize();

        return newqueue;
    }

    VectorCohortWithHabitat::~VectorCohortWithHabitat()
    {
    }

    VectorHabitatType::Enum VectorCohortWithHabitat::GetHabitatType()
    {
        return habitat_type;
    }

    IVectorHabitat* VectorCohortWithHabitat::GetHabitat()
    {
        return habitat;
    }

    void VectorCohortWithHabitat::SetHabitat( IVectorHabitat* new_habitat )
    {
        if ( new_habitat != habitat )
        {
            delete habitat;
            habitat = new_habitat;
        }
    }

    REGISTER_SERIALIZABLE(VectorCohortWithHabitat);

    void VectorCohortWithHabitat::serialize(IArchive& ar, VectorCohortWithHabitat* obj)
    {
        VectorCohort::serialize( ar, obj );
        VectorCohortWithHabitat& cohort = *obj;
        ar.labelElement("habitat_type") & (uint32_t&)cohort.habitat_type;
    }
}
