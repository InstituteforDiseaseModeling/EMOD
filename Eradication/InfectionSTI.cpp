/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "Debug.h"
#include "InfectionSTI.h"
#include "STIInterventionsContainer.h"
#include "Individual.h"
#include "SimulationConfig.h"
#include "RANDOM.h"

static const char* _module = "InfectionSTI";

namespace Kernel
{
    InfectionSTI *InfectionSTI::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        InfectionSTI *newinfection = _new_ InfectionSTI(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    InfectionSTI::~InfectionSTI(void) { }
    InfectionSTI::InfectionSTI() { }
    InfectionSTI::InfectionSTI(IIndividualHumanContext *context) : Infection(context) { }

    float
    InfectionSTI::GetInfectiousness()
    const
    {
        // DJK TODO: use generic linear interpolation code instead of repeating here <ERAD-1877>
        // DJK TODO: use search instead of linear <ERAD-1878>

        // If in relationship, multiply this by co-infection flag of EITHER partner, but not twice.
        //IRelationship * pRel = parent->GetRelationships();
        // Do this every time, or once for all? Afraid of impact of premature attenuation on disease progress, etc.
        // TBD: Nasty cast
        float retInf = infectiousness;

        LOG_DEBUG_F( "raw infectiousness = %f, modified = %f\n", (float)infectiousness, (float)retInf );
        //release_assert( retInf>0.0f );
        return retInf;
    }

    void InfectionSTI::Update(
        float dt,
        Susceptibility* immunity
    )
    { 
        Infection::Update( dt, immunity );
    }
}
 
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::InfectionSTI)
namespace Kernel
{
    template void serialize( boost::mpi::packed_oarchive& ar, InfectionSTI& inf, const unsigned int file_version );
    template void serialize( boost::archive::binary_oarchive& ar, InfectionSTI& inf, const unsigned int file_version );
    template void serialize( boost::mpi::packed_skeleton_oarchive&, Kernel::InfectionSTI&, unsigned int);
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::InfectionSTI&, unsigned int);
    template void serialize( boost::archive::binary_iarchive&, Kernel::InfectionSTI&, unsigned int);
    template void serialize( boost::mpi::packed_iarchive&, Kernel::InfectionSTI&, unsigned int);
    template void serialize( boost::mpi::detail::mpi_datatype_oarchive&, Kernel::InfectionSTI&, unsigned int);
    template void serialize( boost::mpi::detail::content_oarchive&, Kernel::InfectionSTI&, unsigned int);

    template<class Archive>
    void serialize(Archive & ar, InfectionSTI& inf, const unsigned int file_version )
    {
        ar & boost::serialization::base_object<Kernel::Infection>(inf);
    }
}
template void Kernel::serialize(boost::mpi::packed_skeleton_iarchive&, Kernel::InfectionSTI&, unsigned int);
template void Kernel::serialize(boost::archive::binary_iarchive&, Kernel::InfectionSTI&, unsigned int);
template void Kernel::serialize(boost::mpi::packed_iarchive&, Kernel::InfectionSTI&, unsigned int);
template void Kernel::serialize(boost::mpi::packed_skeleton_oarchive&, Kernel::InfectionSTI&, unsigned int);
template void Kernel::serialize(boost::mpi::detail::content_oarchive&, Kernel::InfectionSTI&, unsigned int);
template void Kernel::serialize(boost::mpi::packed_oarchive&, Kernel::InfectionSTI&, unsigned int);
template void Kernel::serialize(boost::mpi::detail::mpi_datatype_oarchive&, Kernel::InfectionSTI&, unsigned int);
template void Kernel::serialize(boost::archive::binary_oarchive&, Kernel::InfectionSTI&, unsigned int);
#endif
