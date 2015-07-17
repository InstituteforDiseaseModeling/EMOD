/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "stdafx.h"

#include "BoostLibWrapper.h"

template<class DerivedT, class BaseT>
struct StaticVoidCastRegistrar
{
    StaticVoidCastRegistrar() { boost::serialization::void_cast_register<DerivedT, BaseT>(static_cast<DerivedT*>(NULL), static_cast<BaseT*>(NULL)); }
};

#define REGISTER_SERIALIZATION_VOID_CAST(classname, interfacename) \
    StaticVoidCastRegistrar<classname, interfacename> BOOST_PP_CAT(static_void_cast_registrar_, BOOST_PP_CAT(classname, BOOST_PP_CAT(_, interfacename)));
