/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "InfectionVector.h"

#include "RapidJsonImpl.h"

static const char* _module = "InfectionVector";

namespace Kernel
{
    InfectionVector::InfectionVector() : Kernel::Infection()
    {
    }

    InfectionVector::InfectionVector(IIndividualHumanContext *context) : Kernel::Infection(context)
    {
    }

    void InfectionVector::Initialize(suids::suid _suid)
    {
        Kernel::Infection::Initialize(_suid);
    }

    InfectionVector *InfectionVector::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        InfectionVector *newinfection = _new_ InfectionVector(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    InfectionVector::~InfectionVector()
    {
    }

#if 0
    template<class Archive>
    void InfectionVector::serialize( Archive & ar, const unsigned int file_version )
    {
        // Register derived types - N/A
        // Serialize fields - N/A

        // Serialize base class
        ar & boost::serialization::base_object<Kernel::Infection>(*this);
    }

    template void InfectionVector::serialize( boost::archive::binary_iarchive & ar, const unsigned int file_version );
    template void InfectionVector::serialize( boost::archive::binary_oarchive & ar, const unsigned int file_version );
    template void InfectionVector::serialize( boost::mpi::packed_iarchive & ar, const unsigned int file_version );
    template void InfectionVector::serialize( boost::mpi::packed_oarchive & ar, const unsigned int file_version );
#endif
}

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
namespace Kernel {

    
    // IJsonSerializable Interfaces
    void InfectionVector::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();
        root->Insert("Infection");
        Infection::JSerialize( root, helper );
        root->EndObject();

    }
        
    void InfectionVector::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
        rapidjson::Document * doc = (rapidjson::Document*) root;
        
        //LOG_INFO_F( "8. %s\n", __FUNCTION__);

        Infection::JDeserialize( (IJsonObjectAdapter*) &((*doc)["Infection"]), helper);
    }


} // namespace Kernel
#endif


#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::InfectionVector)
namespace Kernel
{
    template<class Archive>
    void serialize(Archive & ar, InfectionVector& inf, unsigned int file_version )
    {
        ar & boost::serialization::base_object<Kernel::Infection>(inf);
    }
    template void serialize( boost::archive::binary_iarchive&, InfectionVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_iarchive&, InfectionVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_skeleton_oarchive&, InfectionVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_skeleton_iarchive&, InfectionVector &obj, unsigned int file_version );
    template void serialize( boost::archive::binary_oarchive&, InfectionVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_oarchive&, InfectionVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::detail::content_oarchive&, InfectionVector &obj, unsigned int file_version );
    template void serialize( boost::mpi::detail::mpi_datatype_oarchive&, InfectionVector &obj, unsigned int file_version );
}

#endif
