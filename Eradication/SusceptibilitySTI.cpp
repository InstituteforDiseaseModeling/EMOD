/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "SusceptibilitySTI.h"

namespace Kernel
{
    SusceptibilitySTI *SusceptibilitySTI::CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod)
    {
        SusceptibilitySTI *newsusceptibility = _new_ SusceptibilitySTI(context);
        newsusceptibility->Initialize(age, immmod, riskmod);

        return newsusceptibility;
    }

    SusceptibilitySTI::~SusceptibilitySTI(void) { }
    SusceptibilitySTI::SusceptibilitySTI() { }
    SusceptibilitySTI::SusceptibilitySTI(IIndividualHumanContext *context) : Susceptibility(context) { }

    void SusceptibilitySTI::Initialize(float _age, float _immmod, float _riskmod)
    {
        Susceptibility::Initialize(_age, _immmod, _riskmod);

        // TODO: what are we doing here? 
        // initialize members of airborne susceptibility below
        demographic_risk = _riskmod; 
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::SusceptibilitySTI)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, SusceptibilitySTI& sus, const unsigned int file_version )
    {
        ar & sus.demographic_risk;
        ar & boost::serialization::base_object<Susceptibility>(sus);
    }
    template void serialize(boost::archive::binary_iarchive&, Kernel::SusceptibilitySTI&, unsigned int);
    template void serialize(boost::mpi::packed_iarchive&, Kernel::SusceptibilitySTI&, unsigned int);
    template void serialize(boost::mpi::packed_skeleton_iarchive&, Kernel::SusceptibilitySTI&, unsigned int);
    template void serialize(boost::mpi::packed_oarchive&, Kernel::SusceptibilitySTI&, unsigned int);
    template void serialize(boost::mpi::detail::mpi_datatype_oarchive&, Kernel::SusceptibilitySTI&, unsigned int);
    template void serialize(boost::mpi::packed_skeleton_oarchive&, Kernel::SusceptibilitySTI&, unsigned int);
    template void serialize(boost::mpi::detail::content_oarchive&, Kernel::SusceptibilitySTI&, unsigned int);
    template void serialize(boost::archive::binary_oarchive&, Kernel::SusceptibilitySTI&, unsigned int);
}
#endif

