/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

// suids provides services for creating and manipulating a set of lightweight IDs unique to a simulation (Simulation Unique ID ~ suid)
// in a system distributed on multiple processes
// modeled on the boost uuid interface in case we want to extend in that direction eventually

#pragma once

#include "stdint.h"
#include "BoostLibWrapper.h"
#include "Serializer.h"
#include "Configuration.h"

namespace Kernel
{
    namespace suids 
    {
        /*
        Note for users:
        The child object id system can choose a storage size appropriate to the size of the problem. 
        To keep lightweight objects lightweight, we allow of the possibility of relatively short (eg 32bit) identities.
        This may have significant performance advantages in cases where objects may need to track, record, and reports 
        on the ids of multiple other objects. 

        32bits is the default which will be fine for most systems that have << (2 billion)/numtasks of any 
        entity type over their lifetime.

        The suid class is also designed to provide a subset of the boost::uuid interface so that a fairly painless substitution 
        could be made if that is warranted.
        */

        typedef int32_t suid_data_t; 

        class IDMAPI suid : public IJsonSerializable
        {
        public:
            DECLARE_QUERY_INTERFACE()
            IMPLEMENT_ONLY_REFERENCE_COUNTING()

            typedef uint8_t value_type;
            typedef uint8_t& reference;
            typedef uint8_t const& const_reference;
            typedef uint8_t* iterator;
            typedef uint8_t const* const_iterator;
            typedef std::size_t size_type;
            typedef std::ptrdiff_t difference_type;

            static size_type static_size() { return sizeof(suid_data_t); }

            // iteration
            iterator begin() { return (iterator)&data; }
            iterator end() { return (iterator)(&data)+sizeof(suid_data_t); }
            const_iterator begin() const { return (iterator)&data; }
            const_iterator end() const { return (iterator)(&data)+sizeof(suid_data_t); }

            size_type size() const { return sizeof(suid_data_t); }

            // for consistency with boost::uuid, define a special NULL suid. 
            // NB: non-nil generating generators must never return an suid with data==0! 
            bool is_nil() const { return data == 0; }

            /*
            enum variant_type {
                variant_ncs, // NCS backward compatibility
                variant_rfc_4122, // defined in RFC 4122 document
                variant_microsoft, // Microsoft Corporation backward compatibility
                variant_future // future definition
            };
            variant_type variant() const;

            enum version_type {
                version_unknown = -1,
                version_time_based = 1,
                version_dce_security = 2,
                version_name_based_md5 = 3,
                version_random_number_based = 4,
                version_name_based_sha1 = 5
            };
            version_type version() const;
            */

            // Swap function
            void swap(suid& rhs) 
            { 
                suid_data_t temp = data;
                data = rhs.data;
                rhs.data = temp;
            }

            // POD implementation as with boost::uuid
            /*uint8_t data[static_size()];*/
            suid_data_t data;

        public:
            // IJsonSerializable Interfaces
            virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        
            virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );

        private:
            ///////////////////////////////////////////////////////////////////////////
            // Serialization
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int /* file_version */)
            {
                ar & data;
            }
#endif

            ///////////////////////////////////////////////////////////////////////////
        };

        // standard operators
        inline bool operator==(suid const& lhs, suid const& rhs) { return lhs.data == rhs.data; }
        inline bool operator!=(suid const& lhs, suid const& rhs) { return lhs.data != rhs.data; }
        inline bool operator<(suid const& lhs, suid const& rhs)  { return lhs.data <  rhs.data; }
        inline bool operator>(suid const& lhs, suid const& rhs)  { return lhs.data >  rhs.data; }
        inline bool operator<=(suid const& lhs, suid const& rhs) { return lhs.data <= rhs.data; }
        inline bool operator>=(suid const& lhs, suid const& rhs) { return lhs.data >= rhs.data; }

        inline void swap(suid& lhs, suid& rhs)
        { 
            suid_data_t temp = lhs.data;
            lhs.data = rhs.data;
            rhs.data = temp;
        }

        // verbatim from boost/uuid/uuid.hpp
        inline std::size_t hash_value(suid const& u) /* throw() */
        {
            std::size_t seed = 0;
            for (auto element : u)
            {
                seed ^= static_cast<std::size_t>(element) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }

        // generators - patterned after boost uuid generators 

        struct nil_generator 
        {
            typedef suid result_type;

            suid operator()() const { suid nil; nil.data =0; return nil; }
        };
        inline suid nil_suid() { nil_generator ng; return ng(); }

        // for lightweight id objects we want to track by category and maintain some process-specific history
        template<class ObjectCategoryT>
        class distributed_generator 
        {
        public:
            typedef suid result_type;

            distributed_generator(int _rank, int _numtasks) :
            rank(_rank), numtasks(_numtasks)
            {
                next_suid.data = rank+1; // +1 ensures that NIL will never be generated
            }

            suid operator()()
            {
                suid tmp = next_suid;
                next_suid.data += numtasks;
                return tmp;
            }
        private:
            suid next_suid;
            int rank;
            int numtasks;

        private:
            ///////////////////////////////////////////////////////////////////////////
            // Serialization
#if USE_JSON_SERIALIZATION
        public:
            // IJsonSerializable Interfaces
            virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
            virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
        private:
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int /* file_version */)
            {
                ar & next_suid;
                ar & rank;
                ar & numtasks;
            }
#endif
            ///////////////////////////////////////////////////////////////////////////
        };

#if USE_JSON_SERIALIZATION
        // template member function definition has to be in the header file
        template<class ObjectCategoryT> void distributed_generator<ObjectCategoryT>::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
        {
            root->BeginObject();

            helper->JSerialize("next_suid", (IJsonSerializable*)&next_suid, root);

            root->Insert("rank", rank);
            root->Insert("numtasks", numtasks);

            root->EndObject();
        }

        template<class ObjectCategoryT> void distributed_generator<ObjectCategoryT>::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
        {
            IJsonObjectAdapter* next = (*root)["next_suid"];
            helper->JDeserialize((IJsonSerializable*)&next_suid, next);

            rank     = root->GetInt("rank");
            numtasks = root->GetInt("numtasks");
        }
#endif
    } // namespace Kernel::suids
}

// verbatim from boost/uuid/uuid.hpp
#ifndef BOOST_UUID_NO_TYPE_TRAITS
// type traits specializations
namespace boost {

    template <>
    struct is_pod<Kernel::suids::suid> : true_type {};

} // namespace boost

#endif

#if USE_BOOST_SERIALIZATION
BOOST_IS_MPI_DATATYPE(Kernel::suids::suid)
BOOST_CLASS_TRACKING(Kernel::suids::suid,track_never)
BOOST_IS_BITWISE_SERIALIZABLE(Kernel::suids::suid)
#endif
