/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#ifndef WIN32
#include <cxxabi.h>
#endif

#include "Configure.h"

namespace Kernel
{
    // This template allows one to have a collection/vector of JsonConfigurable objects.
    // The object created from this will need to use the initConfigComplexType() since this
    // is a complex object.  The JSON will be an array of the JsonConfigurable object used
    // in the template, i.e. JsonConfigurableClass.
    template<class JsonConfigurableClass>
    class JsonConfigurableCollection : public JsonConfigurable, public IComplexJsonConfigurable
    {
    public:
        JsonConfigurableCollection( const std::string& rIdmTypeName )
            : JsonConfigurable()
            , m_IdmTypeName( rIdmTypeName )
            , m_Collection()
        {
        }

        virtual ~JsonConfigurableCollection()
        {
            for( JsonConfigurableClass* p_jcc : m_Collection )
            {
                delete p_jcc;
            }
        }

        //JsonConfigurable methods
        virtual int32_t AddRef() { return 1; }
        virtual int32_t Release() { return 1; }
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return QueryResult::e_NOINTERFACE; }

        // IComplexJsonConfigurable methods
        virtual bool  HasValidDefault() const override
        {
            return false;
        }

        virtual json::QuickBuilder GetSchema() override
        {
            JsonConfigurableClass* p_jcc = CreateObject();
            if( JsonConfigurable::_dryrun )
            {
                p_jcc->Configure( nullptr );
            }

            std::string idm_type_schema = "idmType:" + m_IdmTypeName;
            std::string object_name = typeid(*p_jcc).name();
#ifdef WIN32
            object_name = object_name.substr( 14 ); // remove "class Kernel::"
#else
            object_name = abi::__cxa_demangle( object_name.c_str(), 0, 0, nullptr );
            object_name = object_name.substr( 8 ); // remove "Kernel::"
#endif
            std::string object_schema_name = "<" + object_name + " Value>";

            json::QuickBuilder schema( GetSchemaBase() );
            auto tn = JsonConfigurable::_typename_label();
            auto ts = JsonConfigurable::_typeschema_label();
            schema[ tn ] = json::String( idm_type_schema );

            schema[ ts ] = json::Object();
            schema[ ts ][ object_schema_name ] = p_jcc->GetSchema();

            delete p_jcc;

            return schema;
        }

        virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override
        {
            // Temporary object created so we can 'operate' on json with the desired tools
            auto p_config = Configuration::CopyFromElement( (*inputJson)[ key ], inputJson->GetDataLocation() );

            const auto& json_array = json_cast<const json::Array&>((*p_config));
            for( auto data = json_array.Begin(); data != json_array.End(); ++data )
            {
                Configuration* p_object_config = Configuration::CopyFromElement( *data, inputJson->GetDataLocation() );

                JsonConfigurableClass* p_jcc = CreateObject();
                p_jcc->Configure( p_object_config );

                Add( p_jcc );

                delete p_object_config;
            }
            delete p_config;
        }

        // Other methods
        virtual void CheckConfiguration()
        {
        }

        virtual void Add( JsonConfigurableClass* pJcc )
        {
            m_Collection.push_back( pJcc );
        }

        int Size() const
        {
            return m_Collection.size();
        }

        JsonConfigurableClass* operator[]( int index )
        {
            return m_Collection[ index ];
        }

    protected:
        virtual JsonConfigurableClass* CreateObject() = 0;

        std::string m_IdmTypeName;
        std::vector<JsonConfigurableClass*> m_Collection;
    };
}