/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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

    VectorCohortAging::VectorCohortAging(float _age, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics) :
        VectorCohort((float)progress, initial_population, _vector_genetics),
        age(_age)
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

#if 0
    template<class Archive>
    void VectorCohortAging::serialize_inner( Archive & ar, const unsigned int file_version )
    {
        // Register derived types - N/A

        // Serialize fields
        typemap.serialize(this, ar, file_version);

        // Serialize base class
        ar & boost::serialization::base_object<VectorCohort>(*this);
    }
#endif

    //template void VectorCohortAging::serialize( boost::archive::binary_iarchive & ar, const unsigned int file_version );
    //template void VectorCohortAging::serialize( boost::archive::binary_oarchive & ar, const unsigned int file_version );

}


#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::VectorCohortAging)
namespace Kernel {
    template< typename Archive >
    void serialize( Archive& ar, VectorCohortAging &obj, unsigned int file_version )
    {
        ar & obj.age;
        ar & boost::serialization::base_object<Kernel::VectorCohort>(obj);
    }
    template void serialize(boost::mpi::packed_iarchive&, Kernel::VectorCohortAging&, unsigned int);
    template void serialize(boost::archive::binary_iarchive&, Kernel::VectorCohortAging&, unsigned int);
    template void serialize(boost::mpi::packed_skeleton_iarchive&, Kernel::VectorCohortAging&, unsigned int);
    template void serialize(boost::archive::binary_oarchive&, Kernel::VectorCohortAging&, unsigned int);
    template void serialize(boost::mpi::detail::content_oarchive&, Kernel::VectorCohortAging&, unsigned int);
    template void serialize(boost::mpi::packed_skeleton_oarchive&, Kernel::VectorCohortAging&, unsigned int);
    template void serialize(boost::mpi::packed_oarchive&, Kernel::VectorCohortAging&, unsigned int);
    template void serialize(boost::mpi::detail::mpi_datatype_oarchive&, Kernel::VectorCohortAging&, unsigned int);
}
#endif
