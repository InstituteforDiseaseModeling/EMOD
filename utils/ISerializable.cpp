/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ISerializable.h"
#include "IArchive.h"

namespace Kernel
{
    SerializationRegistrar* SerializationRegistrar::_singleton = nullptr;

    void ISerializable::serialize(IArchive& ar, ISerializable*& obj)
    {
        static std::string nullptr_string( "nullptr" );

        if ( ar.IsWriter() && ( obj == nullptr ) )
        {
            ar.startClass( nullptr_string );
            ar.endClass();
            return;
        }

        std::string class_name = ar.IsWriter() ? obj->GetClassName() : "__UNK__";
        ar.startClass(class_name);

        if ( ar.IsReader() && (class_name == nullptr_string) )
        {
            ar.endClass();
            obj = nullptr;
            return;
        }

        auto serialize_function = SerializationRegistrar::_get_serializer(class_name);
        if (!ar.IsWriter())
        {
            auto constructor_function = SerializationRegistrar::_get_constructor(class_name);
            if( constructor_function == nullptr )
            {
                std::stringstream msg;
                msg << "Could not find constructor for class_name='" << class_name << "'";
                throw SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
            obj = constructor_function();
        }
        serialize_function(ar, obj);
        ar.endClass();
    }
}