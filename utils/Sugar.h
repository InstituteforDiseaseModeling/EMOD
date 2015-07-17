/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Sugar (Syntactic)

#pragma once

// Environment helpers
#include "Environment.h"
#define EnvPtr Environment::getInstance() // make code a bit more readable using Dll-boundary-crossable Env pointer

//////////////////////////////////////////////////////////
// helpers for config variables 

#define CONFIG_PARAMETER_EXISTS(config_ptr, name) \
 (config_ptr->As<json::Object>().Find(name) != config_ptr->As<json::Object>().End())

///////////////////////////////////////////////////////////
// convenient macros for boost serialization

// hack to force boost to serialize derived classes properly. its is_polymorphic type trait must evaluate to true to obey type registrations
#define FORCE_POLYMORPHIC() \
    virtual void boost_force_polymorphic() { printf("I'm polymorphic now!\n"); } 

// helper to get template methods created in 
#define INSTANTIATE_SERIALIZER(classname, Archive)\
    template void classname::serialize(Archive &ar, const unsigned int file_version); 

// Add archive types here

#define INSTANTIATE_BOOST_SERIALIZATION_HACKS(classname) \
    INSTANTIATE_SERIALIZER(classname, boost::archive::binary_iarchive)\
    INSTANTIATE_SERIALIZER(classname, boost::archive::binary_oarchive)

// this hack for GCC's benefit, this appears to be a known and unsolvable issue in boost.
// use to resolve 'no unique final overrider' errors where gcc cant deduce the result of traits/is_virtual_base_of
// see https://svn.boost.org/trac/boost/ticket/3730

#define DECLARE_VIRTUAL_BASE_OF(base, derived) \
namespace boost \
{\
    template<>    struct is_virtual_base_of<base, derived>: public mpl::true_ {};\
}
