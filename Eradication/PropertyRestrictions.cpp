/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "PropertyRestrictions.h"
#include "Node.h"
#include "IndividualEventContext.h"

static const char * _module = "PropertyRestrictions";

namespace Kernel
{
    PropertyRestrictions::PropertyRestrictions()
    : JsonConfigurable()
    , _restrictions()
    , verified(false)
    {
    }

    int PropertyRestrictions::Size() const
    {
        return _restrictions.size();
    }

    void PropertyRestrictions::ConfigureFromJsonAndKey( const Configuration * inputJson, const std::string& key )
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
            std::map< std::string, std::string > kvp;
            auto json_map = s2sarray[idx].As<json::Object>();
            for( auto data = json_map.Begin();
                      data != json_map.End();
                      ++data )
            {
                std::string key = data->name;
                std::string value = (std::string)s2sarray[idx][key].As< json::String >();
                kvp.insert( std::make_pair( key, value ) );
            }
            _restrictions.push_back( kvp );
        }
    }

    json::QuickBuilder PropertyRestrictions::GetSchema()
    {
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();

        schema[ tn ] = json::String( "idmType:PropertyRestrictions" );
        schema[ ts ] = json::Array();
        schema[ ts ][0] = json::Object();
        schema[ ts ][0]["<key>"] = json::Object();
        schema[ ts ][0]["<key>"][ "type" ] = json::String( "Constrained String" );
        schema[ ts ][0]["<key>"][ "constraints" ] = json::String( "<demographics>::Defaults.Individual_Properties.*.Property.<keys>" );
        schema[ ts ][0]["<key>"][ "description" ] = json::String( "Individual Property Key from demographics file." );
        schema[ ts ][0]["<value>"] = json::Object();
        schema[ ts ][0]["<value>"][ "type" ] = json::String( "String" );
        schema[ ts ][0]["<key>"][ "constraints" ] = json::String( "<demographics>::Defaults.Individual_Properties.*.Value.<keys>" );
        schema[ ts ][0]["<value>"][ "description" ] = json::String( "Individual Property Value from demographics file." );
        return schema;
    }

    void PropertyRestrictions::Add( std::map< std::string, std::string >& rMap )
    {
        _restrictions.push_back( rMap );
    }

    bool PropertyRestrictions::Qualifies( const IIndividualHumanEventContext* pHEC )
    {
        auto * pProp = const_cast<Kernel::IIndividualHumanEventContext*>(pHEC)->GetProperties();
        release_assert( pProp );

        if( !verified )
        {
            for (auto& prop_map : _restrictions)
            {
                for (auto& prop : prop_map)
                {
                    const std::string& szKey = prop.first;
                    const std::string& szVal = prop.second;

                    Node::VerifyPropertyDefinedInDemographics( szKey, szVal );
                }
            }
            verified = true;
        }

        bool qualifies = true;

        // individual has to have one of these properties
        for (auto& prop_map : _restrictions)
        {
            qualifies = false;
            bool meets_property_restriction_criteria = true;
            for (auto& prop : prop_map)
            {
                const std::string& szKey = prop.first;
                const std::string& szVal = prop.second;

                LOG_DEBUG_F( "Applying property restrictions in event coordinator: %s/%s.\n", szKey.c_str(), szVal.c_str() );
                // Every individual has to have a property value for each property key
                if( pProp->find( szKey ) == pProp->end() )
                {
                    throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "properties", szKey.c_str() );
                }
                else if( pProp->at( szKey ) == szVal )
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

    std::string PropertyRestrictions::GetAsString() const
    {
        std::string restriction_str ;
        if( _restrictions.size() > 0 )
        {
            for( auto prop_map : _restrictions )
            {
                restriction_str += "[ ";
                for( auto entry : prop_map )
                {
                    std::string prop = entry.first + ":" + entry.second;
                    restriction_str += "'"+ prop +"', " ;
                }
                restriction_str = restriction_str.substr( 0, restriction_str.length()-2 );
                restriction_str += " ], ";
            }
            restriction_str = restriction_str.substr( 0, restriction_str.length()-2 );
        }
        return restriction_str;
    }
}