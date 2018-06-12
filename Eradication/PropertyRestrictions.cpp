/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "PropertyRestrictions.h"

SETUP_LOGGING( "PropertyRestrictions" )

namespace Kernel
{
    template<class Key, class KeyValue, class Container>
    PropertyRestrictions<Key,KeyValue,Container>::PropertyRestrictions()
    : JsonConfigurable()
    , _restrictions()
    {
    }

    template<class Key, class KeyValue, class Container>
    int PropertyRestrictions<Key, KeyValue, Container>::Size() const
    {
        return _restrictions.size();
    }

    template<class Key, class KeyValue, class Container>
    void PropertyRestrictions<Key, KeyValue, Container>::ConfigureFromJsonAndKey( const Configuration * inputJson, const std::string& key )
    {
        // Make this optional.
        if( inputJson->Exist( key ) == false )
        {
            return;
        }

        // We have a list of json objects. The format/logic is as follows. For input:
        // [
        //  { "Character": "Good", "Income": "High" },
        //  { "Character": "Bad", "Income": "Low" }
        // ]
        // We give the intervention if the individuals has
        // Good Character AND High Income OR Bad Character AND Low Income.
        // So we AND together the elements of each json object and OR together these 
        // calculated truth values of the elements of the json array.
        json::QuickInterpreter s2sarray = (*inputJson)[key].As<json::Array>();
        for( int idx=0; idx < (*inputJson)[key].As<json::Array>().Size(); idx++ )
        {
            Container container;

            auto json_map = s2sarray[idx].As<json::Object>();
            for( auto data = json_map.Begin();
                      data != json_map.End();
                      ++data )
            {
                std::string key = data->name;
                std::string value = (std::string)s2sarray[idx][key].As< json::String >();
                KeyValue kv( key, value );
                container.Add( kv );
            }
            _restrictions.push_back( container );
        }
    }

    template<class Key, class KeyValue, class Container>
    json::QuickBuilder PropertyRestrictions<Key, KeyValue, Container>::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();

        // this is kind of hacky, but there only two types right now.
        if( std::string( typeid(Key).name() ) == "class Kernel::IPKey" )
        {
            schema[ tn ] = json::String( "idmType:PropertyRestrictions" );
        }
        else
        {
            schema[ tn ] = json::String( "idmType:NodePropertyRestrictions" );
        }
        schema[ ts ] = json::Array();
        schema[ ts ][0] = json::Object();
        schema[ ts ][0]["<key>"] = json::Object();
        schema[ ts ][0]["<key>"][ "type" ] = json::String( "Constrained String" );
        schema[ ts ][0]["<key>"][ "constraints" ] = json::String( Key::GetConstrainedStringConstraintKey() );
        schema[ ts ][0]["<key>"][ "description" ] = json::String( Key::GetConstrainedStringDescriptionKey() );
        schema[ ts ][0]["<value>"] = json::Object();
        schema[ ts ][0]["<value>"][ "type" ] = json::String( "String" );
        schema[ ts ][0]["<value>"][ "constraints" ] = json::String( Key::GetConstrainedStringConstraintValue()  );
        schema[ ts ][0]["<value>"][ "description" ] = json::String( Key::GetConstrainedStringDescriptionValue() );
        return schema;
    }

    template<class Key, class KeyValue, class Container>
    void PropertyRestrictions<Key, KeyValue, Container>::Add( std::map< std::string, std::string >& rMap )
    {
        Container container;
        for( auto& entry : rMap )
        {
            KeyValue kv( entry.first, entry.second );
            container.Add( kv );
        }
        _restrictions.push_back( container );
    }

    template<class Key, class KeyValue, class Container>
    bool PropertyRestrictions<Key, KeyValue, Container>::Qualifies( const Container& rPropertiesContainer )
    {
        bool qualifies = true;

        // individual has to have one of these properties
        for( Container& container : _restrictions)
        {
            qualifies = false;
            bool meets_property_restriction_criteria = true;
            for( auto kv : container )
            {
                if( rPropertiesContainer.Contains( kv ) )
                {
                    LOG_DEBUG_F( "Partial property restriction: constraint is %s.\n", kv.ToString().c_str() );
                    continue; // we're good
                }
                else
                {
                    meets_property_restriction_criteria = false;
#ifdef WIN32
                    LOG_DEBUG_F( "Person does not get the intervention because the allowed property is %s and the person is %s.\n", 
                                 kv.ToString().c_str(), rPropertiesContainer.Get( kv.GetKey<Key>() ).ToString().c_str() );
#endif
                    break;
                }
            }
            // If verified, we're done since these are OR-ed together
            if( meets_property_restriction_criteria )
            {
                qualifies = true;
                LOG_DEBUG_F( "Individual meets at least 1 of the OR-ed together property restriction conditions. Not checking the rest.\n" );
                break;
            }
        }

        return qualifies;
    }

    template<class Key, class KeyValue, class Container>
    bool PropertyRestrictions<Key, KeyValue, Container>::Qualifies( const tProperties* pProp )
    {
        release_assert( pProp );

        bool qualifies = true;

        // individual has to have one of these properties
        for( Container& container : _restrictions )
        {
            qualifies = false;
            bool meets_property_restriction_criteria = true;
            for( KeyValue kv : container )
            {
                const std::string& szKey = kv.GetKeyAsString();
                const std::string& szVal = kv.GetValueAsString();

                LOG_DEBUG_F( "Applying property restrictions in event coordinator: %s/%s.\n", szKey.c_str(), szVal.c_str() );
                // Every individual has to have a property value for each property key
                if( pProp->at( szKey ) == szVal )
                {
                    LOG_DEBUG_F( "Person satisfies (partial) property restriction: constraint is %s/%s and the person is %s.\n", szKey.c_str(), szVal.c_str(), pProp->at( szKey ).c_str() );
                    continue; // we're good
                }
                else
                {
                    meets_property_restriction_criteria = false;
                    LOG_DEBUG_F( "Person does not get the intervention because the allowed property is %s/%s and the person is %s.\n", szKey.c_str(), szVal.c_str(), pProp->at( szKey ).c_str() );
                    break;
                }
            }
            // If verified, we're done since these are OR-ed together
            if( meets_property_restriction_criteria )
            {
                qualifies = true;
                LOG_DEBUG_F( "Individual meets at least 1 of the OR-ed together property restriction conditions. Not checking the rest.\n" );
                break;
            }
        }

        return qualifies;
    }

    template<class Key, class KeyValue, class Container>
    std::string PropertyRestrictions<Key, KeyValue, Container>::GetAsString() const
    {
        std::string restriction_str ;
        if( _restrictions.size() > 0 )
        {
            for( Container container : _restrictions )
            {
                restriction_str += "[ ";
                for( KeyValue kv : container )
                {
                    std::string prop = kv.ToString();
                    restriction_str += "'"+ prop +"', " ;
                }
                restriction_str = restriction_str.substr( 0, restriction_str.length()-2 );
                restriction_str += " ], ";
            }
            restriction_str = restriction_str.substr( 0, restriction_str.length()-2 );
        }
        return restriction_str;
    }

    // -------------------------------------------------------------------------------
    // --- This defines the implementations for these templetes with these parameters.
    // --- If you comment these out, you will get unresolved externals when linking.
    // -------------------------------------------------------------------------------
    template class PropertyRestrictions<IPKey, IPKeyValue, IPKeyValueContainer>;
    template class PropertyRestrictions<NPKey, NPKeyValue, NPKeyValueContainer>;

}
