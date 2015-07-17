/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "InfectionAirborne.h"

namespace Kernel
{
    InfectionAirborne *InfectionAirborne::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        InfectionAirborne *newinfection = _new_ InfectionAirborne(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    InfectionAirborne::~InfectionAirborne(void) { }
    InfectionAirborne::InfectionAirborne() { }
    InfectionAirborne::InfectionAirborne(IIndividualHumanContext *context) : Infection(context) { }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::InfectionAirborne)
namespace Kernel
{
    template<class Archive>
    void serialize(Archive & ar, InfectionAirborne& inf, const unsigned int file_version )
    {
        ar & boost::serialization::base_object<Kernel::Infection>(inf);
    }
    template void serialize( boost::mpi::packed_skeleton_oarchive&, Kernel::InfectionAirborne&, unsigned int);
    template void serialize( boost::mpi::detail::content_oarchive&, Kernel::InfectionAirborne&, unsigned int);
    template void serialize( boost::mpi::packed_oarchive&, Kernel::InfectionAirborne&, unsigned int);
    template void serialize( boost::mpi::detail::mpi_datatype_oarchive&, Kernel::InfectionAirborne&, unsigned int);
    template void serialize( boost::archive::binary_oarchive&, Kernel::InfectionAirborne&, unsigned int);
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::InfectionAirborne&, unsigned int);
    template void serialize( boost::archive::binary_iarchive&, Kernel::InfectionAirborne&, unsigned int);
    template void serialize( boost::mpi::packed_iarchive&, Kernel::InfectionAirborne&, unsigned int);
}
#endif

#endif // ENABLE_TB
