/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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
    {
    }

    VectorCohortWithHabitat::VectorCohortWithHabitat(VectorHabitat* _habitat, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics)
        : VectorCohort((float)progress, initial_population, _vector_genetics),
        habitat(_habitat)
    {
    }

    void VectorCohortWithHabitat::Initialize()
    {
        VectorCohort::Initialize();
    }

    VectorCohortWithHabitat *VectorCohortWithHabitat::CreateCohort(VectorHabitat* _habitat, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics)
    {
        VectorCohortWithHabitat *newqueue = _new_ VectorCohortWithHabitat(_habitat, progress, initial_population, _vector_genetics);
        newqueue->Initialize();

        return newqueue;
    }

    VectorCohortWithHabitat::~VectorCohortWithHabitat()
    {
    }

    VectorHabitat* VectorCohortWithHabitat::GetHabitat()
    {
        return habitat;
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::VectorCohortWithHabitat)
namespace Kernel {
    template< typename Archive >
    void serialize( Archive& ar, VectorCohortWithHabitat &obj, unsigned int file_version )
    {
        //ar & obj.habitat; // these guys don't migrate
        ar & boost::serialization::base_object<Kernel::VectorCohort>(obj);
    }
}
#endif