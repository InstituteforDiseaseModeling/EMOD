/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/
#pragma once

#include "Profile.h"

#pragma warning (disable: 4267)
#pragma warning (disable: 4244)
#pragma warning (disable: 4503)
#pragma warning (disable: 4250)

#if USE_BOOST_GENERAL
#include <boost/scoped_ptr.hpp>
#include <boost/mpi.hpp>
#include <boost/bimap.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#endif

#if USE_BOOST_ARCHIVE
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#endif

#if USE_BOOST_SERIALIZATION
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/is_bitwise_serializable.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/export.hpp>

// WARNING: Boost serialization apparently has issues when trying to serialize hash_maps...
// This solution is "less-than-ideal" (read: a big hack), but it does make things work; we
// may want to consider moving to unordered_map at some point in the future, since that
// appears to be part of the C++11 standard, and likely to be better supported going forward
#define BOOST_HAS_HASH
#define __MWERKS__ 0x3000 // yuck!
#define BOOST_HASH_MAP_HEADER <hash_map>
#include <boost/serialization/hash_map.hpp>

#undef __MWERKS__

#endif

#if USE_BOOST_MPI
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/mpi/operations.hpp>
#include <boost/mpi/skeleton_and_content.hpp>
#include <boost/mpi/collectives/broadcast.hpp>

#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/export.hpp>
#endif

#if USE_BOOST_ALGORITHM
#include <boost/algorithm/string.hpp>
#endif

#if USE_BOOST_MATH
#include <boost/math/special_functions/fpclassify.hpp>
#endif

#if USE_BOOST_ASSIGN
#include <boost/assign/list_of.hpp>
#endif

#if USE_BOOST_UUID
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#endif

// Note: It would be nice to have a symmetric usage of disable/default of warning level and it is possible
//       for header file inclusion, but the trouble is sometimes the reference of boost library class instance 
//       in the CPP file directly such as: nodeid_suid_map->right.count (in Climate.cpp, 344 lines)
//       which could trigger a lot of 4503 warnings (not exactly sure why), hence this forbids or destroy
//       general usage of #pragma warning(default: 4503) pair or #pragma push/pop pair

//#pragma warning (default: 4267)
//#pragma warning (default: 4244)
//#pragma warning (default: 4503)