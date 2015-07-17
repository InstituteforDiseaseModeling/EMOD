/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <map>

using namespace std;

namespace Kernel
{
    typedef int RouteIndex;
    typedef int GroupIndex;

    class TransmissionGroupMembership_t : public map<RouteIndex, GroupIndex>
    {
        // TODO - yuck.
    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive& archive, TransmissionGroupMembership_t& human, const unsigned int file_version) {}
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper )  const {}
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )      {}
#endif
    };
}