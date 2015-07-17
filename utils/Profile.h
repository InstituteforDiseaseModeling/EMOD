/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#ifndef USE_BOOST_GENERAL
#define USE_BOOST_GENERAL 1
#endif

#ifndef USE_BOOST_ARCHIVE
#define USE_BOOST_ARCHIVE 1
#endif

#ifndef USE_BOOST_SERIALIZATION
#define USE_BOOST_SERIALIZATION 0
#endif

#ifndef USE_JSON_SERIALIZATION
#define USE_JSON_SERIALIZATION 1
#endif

#ifndef USE_JSON_MPI
#define USE_JSON_MPI 0
#endif

#ifndef USE_BOOST_MPI
#ifdef WIN32
#define USE_BOOST_MPI 1
#endif
#endif

#ifndef USE_BOOST_ALGORITHM
#define USE_BOOST_ALGORITHM 1
#endif

#ifndef USE_BOOST_MATH
#define USE_BOOST_MATH 1
#endif

#ifndef USE_BOOST_ASSIGN
#define USE_BOOST_ASSIGN 1
#endif

#ifndef USE_BOOST_UUID
#define USE_BOOST_UUID 1
#endif
