/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/
#pragma once

#include "Profile.h"

#pragma warning (disable: 4267)
#pragma warning (disable: 4244)
#pragma warning (disable: 4503)
#pragma warning (disable: 4250)

#if USE_BOOST_GENERAL
#include <boost/scoped_ptr.hpp>
#include <boost/bimap.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
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