/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ISupports.h"
#include "Exceptions.h"

#include <map>
#include <string>

#include <stack>        // for _pool declaration/implementation

namespace Kernel {

    struct IArchive;

    struct IDMAPI ISerializable : ISupports
    {
        virtual const char* GetClassName() { throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "subclass needs to implement" ); }
        virtual void Recycle() { throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "subclass needs to implement" ); }
        virtual ~ISerializable() {}

        static void serialize(IArchive&, ISerializable*&);
    };

    struct SerializationRegistrar
    {
        typedef void (*serialize_function_t)(IArchive&, ISerializable*);
        typedef ISerializable* (*constructor_function_t)(void);

        static void _register(const char* classname, serialize_function_t serializer, constructor_function_t constructor)
        {
            if (!_singleton) _singleton = new SerializationRegistrar();
            _singleton->serializer_map[classname] = serializer;
            _singleton->constructor_map[classname] = constructor;
        }

        static serialize_function_t _get_serializer(std::string& classname) { return _singleton->serializer_map[classname]; }
        static constructor_function_t _get_constructor(std::string& classname) { return _singleton->constructor_map[classname]; }
        std::map<std::string, serialize_function_t> serializer_map;
        std::map<std::string, constructor_function_t> constructor_map;
        static SerializationRegistrar* _singleton;
    };

    template<typename Derived>
    struct SerializationRegistrationCaller
    {
        SerializationRegistrationCaller()
        {
            in_class_registration_hook();
        }
        static void in_class_registration_hook()
        {
            // typename breaks gcc -- need to make cross-platform
            SerializationRegistrar::_register(
                /*typename*/ Derived::_class_name,
                [](IArchive& ar, ISerializable* obj)
                {
                    return Derived::serialize(ar, dynamic_cast<Derived*>(obj));
                },
                /*typename*/  Derived::construct); 
        }
    };

    template<typename T>
    struct PoolManager
    {
        static T* _allocate()
        {
            T* _new;
            if (_pool.size() > 0)
            {
                _new = new(_pool.top()) T();
                _pool.pop();
            }
            else
                _new = new T();
            return _new;
        }
        static void _recycle(T* t)
        {
            t->~T();
            _pool.push(t);
        }
    private:
        static std::stack<T*> _pool;
    };

#define DECLARE_SERIALIZABLE(classname)                                             \
    private:                                                                        \
        virtual const char* GetClassName() override { return _class_name; }         \
        static char* _class_name;                                                   \
        friend SerializationRegistrationCaller<classname>;                          \
        static SerializationRegistrationCaller<classname> serialization_registration_caller; \
        friend PoolManager<classname>; \
        static ISerializable* construct() { return dynamic_cast<ISerializable*>(PoolManager<classname>::_allocate()); }    \
        virtual void Recycle() override { PoolManager<classname>::_recycle(this); } \
    protected:\
        static void serialize(IArchive&, classname*);                               \

#define REGISTER_SERIALIZABLE(classname)                                                     \
    char* classname::_class_name = #classname;                                               \
    SerializationRegistrationCaller<classname> classname::serialization_registration_caller; \
    template<> std::stack<classname*> PoolManager<classname>::_pool = std::stack<classname*>();                                   
}
